//
// Created by Bailey Chessum on 8/4/26.
//

#include "arm_kinematics/joint_map/joint_map_blueprint.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace arm_kinematics {

namespace {

// Sentinel for "not assigned to any transmission stage; lives in the pre-transmission affine
// batch (stage 0)". Stored as ssize_t-style int with a sentinel value.
constexpr int kPreTransmissionStage = -1;

// Walk producer chains starting from `iface` and collect every TransmissionInstanceId that's
// transitively required to compute it. Recurses through AffineProjection sources (which
// invariantly resolve to a leaf — Input or Transmission — in one step).
void collect_required_transmissions(
  const TransmissionReachability & reach,
  const StateInterfaceId iface,
  std::unordered_set<StateInterfaceId> & traced,
  std::unordered_set<TransmissionInstanceId> & required)
{
  std::vector<StateInterfaceId> stack;
  stack.push_back(iface);
  while (!stack.empty()) {
    const StateInterfaceId cur = stack.back();
    stack.pop_back();
    if (!traced.insert(cur).second) {
      continue;
    }
    const auto producer = reach.producer_of(cur);
    if (auto * tx = std::get_if<producers::Transmission>(&producer)) {
      if (required.insert(tx->instance_id).second) {
        const auto & instance = reach.analysis().transmissions()[tx->instance_id];
        for (const StateInterfaceId in : instance.input_ids) {
          stack.push_back(in);
        }
      }
    } else if (auto * ap = std::get_if<producers::AffineProjection>(&producer)) {
      stack.push_back(ap->source);
    }
    // Input or monostate → no further tracing.
  }
}

// Topologically sort the required transmissions. Returns the sorted list. The dependency edge
// `T1 → T2` exists iff T2 has an input that's produced (directly or via an AffineProjection
// whose source is a Transmission output) by T1. Determinism is preserved by sorting by
// TransmissionInstanceId at every choice point.
std::vector<TransmissionInstanceId> topo_sort_required_transmissions(
  const TransmissionReachability & reach,
  const std::unordered_set<TransmissionInstanceId> & required)
{
  std::vector<TransmissionInstanceId> sorted_required(required.begin(), required.end());
  std::sort(sorted_required.begin(), sorted_required.end());

  // Build dependency edges.
  std::unordered_map<TransmissionInstanceId, std::vector<TransmissionInstanceId>> dependents;
  std::unordered_map<TransmissionInstanceId, std::size_t> in_degree;
  for (const auto tid : sorted_required) {
    in_degree[tid] = 0;
  }

  for (const auto t2 : sorted_required) {
    const auto & instance = reach.analysis().transmissions()[t2];
    for (const StateInterfaceId in : instance.input_ids) {
      // Walk through AffineProjection chains to find the underlying transmission, if any.
      auto cur = reach.producer_of(in);
      while (true) {
        if (auto * tx = std::get_if<producers::Transmission>(&cur)) {
          const TransmissionInstanceId t1 = tx->instance_id;
          if (required.count(t1) > 0 && t1 != t2) {
            dependents[t1].push_back(t2);
            in_degree[t2]++;
          }
          break;
        } else if (auto * ap = std::get_if<producers::AffineProjection>(&cur)) {
          cur = reach.producer_of(ap->source);
        } else {
          break;  // Input or monostate
        }
      }
    }
  }

  // Kahn's: process zero-in-degree nodes in id order for determinism.
  std::vector<TransmissionInstanceId> ready;
  for (const auto tid : sorted_required) {
    if (in_degree[tid] == 0) {
      ready.push_back(tid);
    }
  }
  std::sort(ready.begin(), ready.end());

  std::vector<TransmissionInstanceId> result;
  result.reserve(sorted_required.size());
  while (!ready.empty()) {
    const TransmissionInstanceId tid = ready.front();
    ready.erase(ready.begin());
    result.push_back(tid);
    auto deps_it = dependents.find(tid);
    if (deps_it == dependents.end()) continue;
    for (const TransmissionInstanceId dep : deps_it->second) {
      if (--in_degree[dep] == 0) {
        const auto pos = std::lower_bound(ready.begin(), ready.end(), dep);
        ready.insert(pos, dep);
      }
    }
  }
  assert(result.size() == sorted_required.size() &&
         "topological sort produced a partial order — cycle in required transmissions?");
  return result;
}

// Walk a producer chain to find the underlying leaf-Transmission's id, if any. Returns
// std::nullopt if the leaf is an Input. The leaf-source invariant guarantees the chain is at
// most depth 1 (AffineProjection → Input/Transmission), but we walk defensively.
std::optional<TransmissionInstanceId> find_leaf_transmission(
  const TransmissionReachability & reach,
  const StateInterfaceId iface)
{
  auto cur = reach.producer_of(iface);
  while (true) {
    if (auto * tx = std::get_if<producers::Transmission>(&cur)) {
      return tx->instance_id;
    } else if (auto * ap = std::get_if<producers::AffineProjection>(&cur)) {
      cur = reach.producer_of(ap->source);
    } else {
      return std::nullopt;  // Input or monostate
    }
  }
}

}  // namespace

// Error-handling policy for this file:
//   - `assert` is for defensive impossible-state checks: invariants the algorithm guarantees,
//     not user-facing preconditions. Failing one is a programming error in the algorithm
//     itself.
//   - `throw std::logic_error` is for caller precondition violations — when a caller passes
//     bad input (e.g. an output with no producer in the reachability) we throw so it's loud
//     in both debug and release builds.

JointMapBlueprint plan_joint_map(
  const TransmissionReachability & reach,
  const span<const StateInterfaceId> ordered_outputs)
{
  // Caller's contract: the reachability MAY have ambiguous interfaces unrelated to the
  // requested outputs, but every output in `ordered_outputs` must be derivable. The caller
  // is expected to have run `diagnose_missing_outputs` first and verified that both
  // `unreachable` and `ambiguous_outputs` are empty. The per-output check inside the
  // bucketing loop below throws `std::logic_error` if this contract is violated.

  JointMapBlueprint blueprint{};
  {
    std::vector<StateInterfaceId> in_copy(reach.inputs().begin(), reach.inputs().end());
    blueprint.set_inputs(std::move(in_copy));
  }
  {
    std::vector<StateInterfaceId> out_copy(ordered_outputs.begin(), ordered_outputs.end());
    blueprint.set_outputs(std::move(out_copy));
  }

  if (ordered_outputs.empty()) {
    return blueprint;
  }

  // ---------------------------------------------------------------------------
  // Step 1: Determine which transmissions are required, and topologically sort them.
  // ---------------------------------------------------------------------------
  std::unordered_set<TransmissionInstanceId> required;
  {
    std::unordered_set<StateInterfaceId> traced;
    for (const StateInterfaceId out : ordered_outputs) {
      collect_required_transmissions(reach, out, traced, required);
    }
  }
  const auto topo_order = topo_sort_required_transmissions(reach, required);

  // Map TransmissionInstanceId → its position in topo_order, for stage assignment.
  std::unordered_map<TransmissionInstanceId, std::size_t> stage_index_of;
  stage_index_of.reserve(topo_order.size());
  for (std::size_t i = 0; i < topo_order.size(); ++i) {
    stage_index_of[topo_order[i]] = i;
  }

  // ---------------------------------------------------------------------------
  // Step 2: Assign each requested output to a stage.
  //
  // - `Input` producer → kPreTransmissionStage (lives in InputAffineBatch_0)
  // - `AffineProjection` from Input source → kPreTransmissionStage
  // - `AffineProjection` from Transmission T_i source → after T_i runs (stage_index of T_i)
  // - `Transmission T_i` producer → goes inside T_i's TransmissionStage segment, not an
  //   AffineBatch. Tracked separately.
  // ---------------------------------------------------------------------------
  // For each output, the stage at which its AffineBatch row should land. -1 = pre-stage.
  // For outputs that are direct transmission outputs, this is unused; instead they go in
  // direct_transmission_outputs[stage_index].
  const std::size_t output_count = ordered_outputs.size();
  std::vector<int> affine_batch_stage(output_count, kPreTransmissionStage);
  // For each output: true iff its producer is a direct Transmission (not an AffineProjection).
  std::vector<bool> is_direct_transmission(output_count, false);
  // For each output that's a direct transmission output: which transmission produces it.
  std::vector<TransmissionInstanceId> direct_transmission_id(output_count, 0);

  for (std::size_t i = 0; i < output_count; ++i) {
    const StateInterfaceId out = ordered_outputs[i];
    const auto producer = reach.producer_of(out);
    if (std::holds_alternative<std::monostate>(producer)) {
      // Caller precondition violation: this output is unreachable, ambiguous, or blocked.
      // The caller should have caught it via diagnose_missing_outputs and refused to call us.
      throw std::logic_error(
        "plan_joint_map: output has no producer in the reachability "
        "(caller did not gate on diagnose_missing_outputs)");
    }
    if (auto * tx = std::get_if<producers::Transmission>(&producer)) {
      is_direct_transmission[i] = true;
      direct_transmission_id[i] = tx->instance_id;
    } else if (auto * ap = std::get_if<producers::AffineProjection>(&producer)) {
      // Find the underlying leaf to determine which stage produces the source value.
      const auto src_tx = find_leaf_transmission(reach, ap->source);
      if (src_tx.has_value()) {
        affine_batch_stage[i] = static_cast<int>(stage_index_of[*src_tx]);
      } else {
        affine_batch_stage[i] = kPreTransmissionStage;
      }
    }
    // Input → stays at kPreTransmissionStage.
  }

  // ---------------------------------------------------------------------------
  // Step 3: Emit segments in execution order.
  //
  // Layout: [pre-batch] [Tstage_0] [batch_0] [Tstage_1] [batch_1] ...
  //   pre-batch    = outputs with affine_batch_stage == -1, NOT direct transmission outputs
  //   Tstage_k     = required transmission topo_order[k], with the wanted-output mapping
  //                  computed below
  //   batch_k      = outputs with affine_batch_stage == k (i.e. AffineProjections from T_k's
  //                  outputs)
  // ---------------------------------------------------------------------------

  auto emit_affine_batch_for_stage = [&](int stage) {
    JointMapBlueprintSegment::InputAffineBatch batch{};
    for (std::size_t i = 0; i < output_count; ++i) {
      if (is_direct_transmission[i]) continue;
      if (affine_batch_stage[i] != stage) continue;

      const StateInterfaceId out = ordered_outputs[i];
      const auto producer = reach.producer_of(out);
      if (auto * input_p = std::get_if<producers::Input>(&producer)) {
        // Direct input passthrough — identity row, source is the input itself.
        batch.blueprint_output_indices.push_back(i);
        batch.sources.push_back(out);  // The output IS the input state interface in this case
        batch.multipliers.push_back(1.0F);
        batch.offsets.push_back(0.0F);
        (void)input_p;
      } else if (auto * ap = std::get_if<producers::AffineProjection>(&producer)) {
        batch.blueprint_output_indices.push_back(i);
        batch.sources.push_back(ap->source);
        batch.multipliers.push_back(ap->multiplier);
        batch.offsets.push_back(ap->offset);
      }
    }
    if (!batch.blueprint_output_indices.empty()) {
      blueprint.emplace_segment(JointMapBlueprintSegment{std::move(batch)});
    }
  };

  auto emit_transmission_stage = [&](const std::size_t stage_idx) {
    const TransmissionInstanceId tid = topo_order[stage_idx];
    const auto & instance = reach.analysis().transmissions()[tid];

    JointMapBlueprintSegment::TransmissionStage stage{};
    stage.instance_id = tid;
    stage.inputs.assign(instance.input_ids.begin(), instance.input_ids.end());
    stage.outputs.assign(instance.output_ids.begin(), instance.output_ids.end());
    stage.blueprint_output_indices.assign(instance.output_ids.size(), std::nullopt);

    // For each transmission output, check whether it's directly requested.
    for (std::size_t out_pos = 0; out_pos < instance.output_ids.size(); ++out_pos) {
      const StateInterfaceId tx_out = instance.output_ids[out_pos];
      // Is tx_out in ordered_outputs and assigned to this transmission as a direct producer?
      for (std::size_t i = 0; i < output_count; ++i) {
        if (!is_direct_transmission[i]) continue;
        if (direct_transmission_id[i] != tid) continue;
        if (ordered_outputs[i] != tx_out) continue;
        stage.blueprint_output_indices[out_pos] = i;
        break;
      }
    }

    blueprint.emplace_segment(JointMapBlueprintSegment{std::move(stage)});
  };

  // Pre-transmission affine batch.
  emit_affine_batch_for_stage(kPreTransmissionStage);

  // Interleave transmission stages with their post-batches.
  for (std::size_t k = 0; k < topo_order.size(); ++k) {
    emit_transmission_stage(k);
    emit_affine_batch_for_stage(static_cast<int>(k));
  }

  return blueprint;
}

}  // namespace arm_kinematics
