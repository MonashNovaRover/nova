//
// Created by Bailey Chessum on 8/4/26.
//

#include "arm_kinematics/joint_map/joint_map_blueprint.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <variant>
#include <vector>

namespace arm_kinematics {

using StateInterfaceId = BlueprintStateInterfaceId;

namespace {

// Sentinel for "not assigned to any transmission stage; lives in the pre-transmission affine
// batch (stage 0)". Stored as ssize_t-style int with a sentinel value.
constexpr int kPreTransmissionStage = -1;
constexpr std::size_t kUnassignedStageIndex = std::numeric_limits<std::size_t>::max();

// Walk producer chains starting from `def` and collect every TransmissionInstanceId that's
// transitively required to compute it. Recurses through AffineProjection sources (which
// invariantly resolve to a leaf — Input or Transmission — in one step).
//
// Uses a two-tier visited set: vector<bool> indexed by analysis StateInterfaceId for registered
// interfaces (O(1) read/write, no hashing), and unordered_set<StateInterfaceDefinition> only for
// bare (unregistered) definitions. Transmission inputs are pushed as SIDs directly, avoiding the
// inverse-table lookup and re-hashing that the old definition-based stack caused.
void collect_required_transmissions(
  const TransmissionReachability & reach,
  const StateInterfaceDefinition & start_def,
  std::vector<bool> & traced_sids,
  std::unordered_set<StateInterfaceDefinition> & traced_bare,
  std::unordered_set<TransmissionInstanceId> & required)
{
  using AnalysisSID = TransmissionAnalysis::StateInterfaceId;
  using Item = std::variant<AnalysisSID, StateInterfaceDefinition>;

  const auto & analysis = reach.analysis();
  std::vector<Item> stack;

  const auto opt_start_sid = analysis.find_state_interface_id(start_def);
  if (opt_start_sid.has_value()) {
    stack.emplace_back(*opt_start_sid);
  } else {
    stack.emplace_back(start_def);
  }

  while (!stack.empty()) {
    const Item cur = stack.back();
    stack.pop_back();

    producers::StateInterfaceProducer producer;

    if (const auto * sid_ptr = std::get_if<AnalysisSID>(&cur)) {
      const AnalysisSID sid = *sid_ptr;
      if (traced_sids[sid]) {
        continue;
      }
      traced_sids[sid] = true;
      producer = reach.producer_of(sid);
    } else {
      const auto & bare_def = std::get<StateInterfaceDefinition>(cur);
      if (!traced_bare.insert(bare_def).second) {
        continue;
      }
      producer = reach.producer_of_def(bare_def);
    }

    if (auto * tx = std::get_if<producers::Transmission>(&producer)) {
      if (required.insert(tx->instance_id).second) {
        const auto & instance = analysis.transmissions()[tx->instance_id];
        for (const AnalysisSID in_sid : instance.input_ids) {
          stack.emplace_back(in_sid);  // push SID directly — no inverse table lookup
        }
      }
    } else if (auto * ap = std::get_if<producers::AffineProjection>(&producer)) {
      const auto src_sid = analysis.find_state_interface_id(ap->source);
      if (src_sid.has_value()) {
        stack.emplace_back(*src_sid);
      } else {
        stack.emplace_back(ap->source);
      }
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
  const std::size_t transmission_count = reach.analysis().transmissions().size();
  std::vector<std::vector<TransmissionInstanceId>> dependents(transmission_count);
  std::vector<std::size_t> in_degree(transmission_count, 0);

  for (const auto t2 : sorted_required) {
    const auto & instance = reach.analysis().transmissions()[t2];
    for (const StateInterfaceId in : instance.input_ids) {
      // Walk through AffineProjection chains to find the underlying transmission, if any.
      // producer_of(sid) for the SID-indexed real inputs, then follow via producer_of_def.
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
          cur = reach.producer_of_def(ap->source);
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
  std::size_t ready_cursor = 0;
  while (ready_cursor < ready.size()) {
    const TransmissionInstanceId tid = ready[ready_cursor++];
    result.push_back(tid);
    for (const TransmissionInstanceId dep : dependents[tid]) {
      if (--in_degree[dep] == 0) {
        const auto pos = std::lower_bound(ready.begin() + static_cast<std::ptrdiff_t>(ready_cursor), ready.end(), dep);
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
  const StateInterfaceDefinition & def)
{
  auto cur = reach.producer_of_def(def);
  while (true) {
    if (auto * tx = std::get_if<producers::Transmission>(&cur)) {
      return tx->instance_id;
    } else if (auto * ap = std::get_if<producers::AffineProjection>(&cur)) {
      cur = reach.producer_of_def(ap->source);
    } else {
      return std::nullopt;  // Input or monostate
    }
  }
}

// Scratch-fill row: pre-fill a scratch slot for a transmission input whose producer is an
// AffineProjection from a bare source (Case 3). The slot for `target_def` is filled from
// `source_def` via the given affine coefficients before the TransmissionStage runs.
struct ScratchFillRow {
  StateInterfaceDefinition target_def{};
  StateInterfaceDefinition source_def{};
  double multiplier = 1.0;
  double offset = 0.0;
};

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
  const span<const StateInterfaceDefinition> ordered_outputs)
{
  // Caller's contract: the reachability MAY have ambiguous interfaces unrelated to the
  // requested outputs, but every output in `ordered_outputs` must be derivable. The caller
  // is expected to have run `diagnose_missing_outputs` first and verified that the
  // diagnosis's `ok()` returns true. The per-output check inside the bucketing loop below
  // throws `std::logic_error` if this contract is violated.

  const auto & analysis = reach.analysis();

  JointMapBlueprint blueprint{};
  {
    std::vector<StateInterfaceDefinition> in_copy(reach.inputs().begin(), reach.inputs().end());
    blueprint.set_inputs(std::move(in_copy));
  }
  {
    std::vector<StateInterfaceDefinition> out_copy(ordered_outputs.begin(), ordered_outputs.end());
    blueprint.set_outputs(std::move(out_copy));
  }

  // Ensure a blueprint-local id for a definition, recording the analysis SID when available.
  const auto ensure_blueprint_state_id = [&](const StateInterfaceDefinition & def) -> StateInterfaceId {
    return blueprint.ensure_state_interface(BlueprintStateInterfaceRecord{
      def,
      analysis.find_state_interface_id(def)
    });
  };

  // Variant for transmission I/O where the analysis SID is already known — avoids a redundant
  // find_state_interface_id call (the def is obtained from the inverse table but the SID is
  // already in hand, so re-hashing the def to recover it is unnecessary).
  const auto ensure_blueprint_state_id_for_sid =
    [&](const TransmissionAnalysis::StateInterfaceId analysis_sid) -> StateInterfaceId {
      return blueprint.ensure_state_interface(BlueprintStateInterfaceRecord{
        analysis.state_interface_order().inverse[analysis_sid],
        analysis_sid
      });
    };

  if (ordered_outputs.empty()) {
    return blueprint;
  }

  // ---------------------------------------------------------------------------
  // Step 1: Determine which transmissions are required, and topologically sort them.
  // ---------------------------------------------------------------------------
  std::unordered_set<TransmissionInstanceId> required;
  {
    const std::size_t sid_count = analysis.state_interface_order().size();
    std::vector<bool> traced_sids(sid_count, false);
    std::unordered_set<StateInterfaceDefinition> traced_bare;
    for (const StateInterfaceDefinition & out : ordered_outputs) {
      collect_required_transmissions(reach, out, traced_sids, traced_bare, required);
    }
  }
  const auto topo_order = topo_sort_required_transmissions(reach, required);

  // ---------------------------------------------------------------------------
  // Pure-affine fast path: no transmissions required — skip blueprint-local canonicalization
  // entirely. Build a single input-slot lookup (one hash insert per input) and populate
  // direct_input_slots in the output batch. This avoids the O(N) ensure_blueprint_state_id
  // calls (each of which hashes the definition twice and rehashes the Order map) and the
  // corresponding O(N²) input-slot scan in materialize_pure_affine.
  // ---------------------------------------------------------------------------
  if (topo_order.empty()) {
    std::unordered_map<StateInterfaceDefinition, std::size_t> input_slot_of;
    const auto pa_inputs = reach.inputs();
    input_slot_of.reserve(pa_inputs.size());
    for (std::size_t i = 0; i < pa_inputs.size(); ++i) {
      input_slot_of.emplace(pa_inputs[i], i);
    }

    const std::size_t pa_output_count = ordered_outputs.size();
    JointMapBlueprintSegment::InputAffineBatch batch{};
    batch.blueprint_output_indices.reserve(pa_output_count);
    batch.direct_input_slots.reserve(pa_output_count);
    batch.multipliers.reserve(pa_output_count);
    batch.offsets.reserve(pa_output_count);

    for (std::size_t i = 0; i < pa_output_count; ++i) {
      const StateInterfaceDefinition & out = ordered_outputs[i];
      const auto producer = reach.producer_of_def(out);
      const StateInterfaceDefinition * src_def = nullptr;
      double m = 1.0;
      double o = 0.0;
      if (std::get_if<producers::Input>(&producer)) {
        src_def = &out;
      } else if (const auto * ap = std::get_if<producers::AffineProjection>(&producer)) {
        src_def = &ap->source;
        m = ap->multiplier;
        o = ap->offset;
      } else {
        throw std::logic_error(
          "plan_joint_map: pure-affine output has no producer in the reachability "
          "(caller did not gate on diagnose_missing_outputs)");
      }
      const auto it = input_slot_of.find(*src_def);
      if (it == input_slot_of.end()) {
        throw std::logic_error(
          "plan_joint_map: pure-affine output source is not among the reachability inputs");
      }
      batch.blueprint_output_indices.push_back(i);
      batch.direct_input_slots.push_back(it->second);
      batch.multipliers.push_back(m);
      batch.offsets.push_back(o);
    }
    if (!batch.blueprint_output_indices.empty()) {
      blueprint.emplace_segment(JointMapBlueprintSegment{std::move(batch)});
    }
    return blueprint;
  }

  // Map TransmissionInstanceId → its position in topo_order, for stage assignment.
  std::vector<std::size_t> stage_index_of(analysis.transmissions().size(), kUnassignedStageIndex);
  for (std::size_t i = 0; i < topo_order.size(); ++i) {
    stage_index_of[topo_order[i]] = i;
  }

  // ---------------------------------------------------------------------------
  // Step 2: Pre-compute scratch-fill rows for each transmission stage.
  //
  // A transmission input SID `r` whose producer is AffineProjection{src, m, o} has no scratch
  // slot (it's not a user input and not a transmission output). We pre-fill its def's slot in
  // the batch that precedes the transmission so the gather works normally.
  // ---------------------------------------------------------------------------
  std::vector<std::vector<ScratchFillRow>> scratch_fills(topo_order.size());
  for (std::size_t k = 0; k < topo_order.size(); ++k) {
    const auto & instance = analysis.transmissions()[topo_order[k]];
    for (const StateInterfaceId in_sid : instance.input_ids) {
      const auto producer = reach.producer_of(in_sid);
      if (const auto * ap = std::get_if<producers::AffineProjection>(&producer)) {
        scratch_fills[k].push_back(ScratchFillRow{
          analysis.state_interface_order().inverse[in_sid],
          ap->source,
          ap->multiplier,
          ap->offset
        });
      }
    }
  }

  // ---------------------------------------------------------------------------
  // Step 3: Assign each requested output to a stage.
  //
  // - `Input` producer → kPreTransmissionStage (lives in InputAffineBatch_0)
  // - `AffineProjection` from Input source → kPreTransmissionStage
  // - `AffineProjection` from Transmission T_i source → after T_i runs (stage_index of T_i)
  // - `Transmission T_i` producer → goes inside T_i's TransmissionStage segment, not an
  //   AffineBatch. Tracked separately.
  // ---------------------------------------------------------------------------
  const std::size_t output_count = ordered_outputs.size();
  std::vector<int> affine_batch_stage(output_count, kPreTransmissionStage);
  std::vector<bool> is_direct_transmission(output_count, false);
  std::vector<TransmissionInstanceId> direct_transmission_id(output_count, 0);

  for (std::size_t i = 0; i < output_count; ++i) {
    const StateInterfaceDefinition & out = ordered_outputs[i];
    const auto producer = reach.producer_of_def(out);
    if (std::holds_alternative<std::monostate>(producer)) {
      throw std::logic_error(
        "plan_joint_map: output has no producer in the reachability "
        "(caller did not gate on diagnose_missing_outputs)");
    }
    if (auto * tx = std::get_if<producers::Transmission>(&producer)) {
      is_direct_transmission[i] = true;
      direct_transmission_id[i] = tx->instance_id;
    } else if (auto * ap = std::get_if<producers::AffineProjection>(&producer)) {
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
  // Step 4: Pre-compute per-stage output lists and SID → blueprint-output-index map.
  //
  // These replace two O(N²) scans in the emit loops:
  //   (a) emit_affine_batch_for_stage previously scanned all output_count outputs per stage to
  //       find those assigned to it — O(output_count × stage_count) total.
  //   (b) emit_transmission_stage previously scanned all output_count outputs per tx output to
  //       find whether it was directly requested — O(output_count × total_tx_output_count) total.
  // Both are now O(output_count) precomputation + O(1) per lookup.
  // ---------------------------------------------------------------------------

  // affine_outputs_for_stage[0]     = output indices with affine_batch_stage == kPreTransmissionStage
  // affine_outputs_for_stage[k + 1] = output indices with affine_batch_stage == k
  const std::size_t affine_stage_count = topo_order.size() + 1;
  std::vector<std::vector<std::size_t>> affine_outputs_for_stage(affine_stage_count);
  for (std::size_t i = 0; i < output_count; ++i) {
    if (is_direct_transmission[i]) continue;
    const int s = affine_batch_stage[i];
    const std::size_t bucket =
      (s == kPreTransmissionStage) ? 0u : static_cast<std::size_t>(s + 1);
    affine_outputs_for_stage[bucket].push_back(i);
  }

  // sid_to_blueprint_output[sid] = blueprint output index for that directly-requested tx output.
  // Indexed by analysis StateInterfaceId (dense, zero-based). nullopt if not directly requested.
  const std::size_t analysis_sid_count = analysis.state_interface_order().size();
  std::vector<std::optional<std::size_t>> sid_to_blueprint_output(analysis_sid_count, std::nullopt);
  for (std::size_t i = 0; i < output_count; ++i) {
    if (!is_direct_transmission[i]) continue;
    const auto opt_sid = analysis.find_state_interface_id(ordered_outputs[i]);
    if (opt_sid.has_value()) {
      sid_to_blueprint_output[*opt_sid] = i;
    }
  }

  // ---------------------------------------------------------------------------
  // Step 5: Emit segments in execution order.
  //
  // Layout: [pre-batch] [Tstage_0] [batch_0] [Tstage_1] [batch_1] ...
  //   pre-batch    = outputs with affine_batch_stage == -1, NOT direct transmission outputs,
  //                  PLUS scratch-fill rows for Tstage_0
  //   Tstage_k     = required transmission topo_order[k]
  //   batch_k      = outputs with affine_batch_stage == k,
  //                  PLUS scratch-fill rows for Tstage_{k+1}
  // ---------------------------------------------------------------------------

  auto emit_affine_batch_for_stage = [&](
    const std::vector<std::size_t> & output_indices,
    const std::vector<ScratchFillRow> & fills)
  {
    JointMapBlueprintSegment::InputAffineBatch batch{};

    for (const std::size_t i : output_indices) {
      const StateInterfaceDefinition & out = ordered_outputs[i];
      const auto producer = reach.producer_of_def(out);
      if (std::get_if<producers::Input>(&producer)) {
        batch.blueprint_output_indices.push_back(i);
        batch.sources.push_back(ensure_blueprint_state_id(out));
        batch.multipliers.push_back(1.0);
        batch.offsets.push_back(0.0);
      } else if (auto * ap = std::get_if<producers::AffineProjection>(&producer)) {
        batch.blueprint_output_indices.push_back(i);
        batch.sources.push_back(ensure_blueprint_state_id(ap->source));
        batch.multipliers.push_back(ap->multiplier);
        batch.offsets.push_back(ap->offset);
      }
    }

    for (const auto & sf : fills) {
      batch.scratch_targets.push_back(ensure_blueprint_state_id(sf.target_def));
      batch.scratch_sources.push_back(ensure_blueprint_state_id(sf.source_def));
      batch.scratch_multipliers.push_back(sf.multiplier);
      batch.scratch_offsets.push_back(sf.offset);
    }

    if (!batch.blueprint_output_indices.empty() || !batch.scratch_targets.empty()) {
      blueprint.emplace_segment(JointMapBlueprintSegment{std::move(batch)});
    }
  };

  auto emit_transmission_stage = [&](const std::size_t stage_idx) {
    const TransmissionInstanceId tid = topo_order[stage_idx];
    const auto & instance = analysis.transmissions()[tid];

    JointMapBlueprintSegment::TransmissionStage stage{};
    stage.instance_id = tid;
    stage.inputs.reserve(instance.input_ids.size());
    for (const auto sid : instance.input_ids) {
      stage.inputs.push_back(ensure_blueprint_state_id_for_sid(sid));
    }
    stage.outputs.reserve(instance.output_ids.size());
    stage.blueprint_output_indices.resize(instance.output_ids.size(), std::nullopt);
    for (std::size_t out_pos = 0; out_pos < instance.output_ids.size(); ++out_pos) {
      const auto sid = instance.output_ids[out_pos];
      stage.outputs.push_back(ensure_blueprint_state_id_for_sid(sid));
      // O(1): look up precomputed sid → blueprint output position.
      if (sid < analysis_sid_count) {
        stage.blueprint_output_indices[out_pos] = sid_to_blueprint_output[sid];
      }
    }

    blueprint.emplace_segment(JointMapBlueprintSegment{std::move(stage)});
  };

  // topo_order is non-empty here (pure-affine case returned early above).
  emit_affine_batch_for_stage(affine_outputs_for_stage[0], scratch_fills[0]);

  const std::vector<ScratchFillRow> empty_fills{};
  for (std::size_t k = 0; k < topo_order.size(); ++k) {
    emit_transmission_stage(k);
    const std::vector<ScratchFillRow> & next_fills =
      (k + 1 < topo_order.size()) ? scratch_fills[k + 1] : empty_fills;
    emit_affine_batch_for_stage(affine_outputs_for_stage[k + 1], next_fills);
  }

  return blueprint;
}

}  // namespace arm_kinematics
