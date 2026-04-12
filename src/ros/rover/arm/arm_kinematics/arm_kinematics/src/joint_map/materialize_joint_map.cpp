//
// Created by Bailey Chessum on 8/4/26.
//

#include "arm_kinematics/joint_map/materialize_joint_map.hpp"

// Error-handling policy for this file:
//   - `assert` is for defensive impossible-state checks: invariants the algorithm guarantees,
//     not user-facing preconditions. Failing one is a programming error in the algorithm
//     itself.
//   - `throw std::logic_error` (or `std::invalid_argument` for shape errors) is for caller
//     precondition violations and shape mismatches — when a caller passes bad input we throw
//     so it's loud in both debug and release builds.

#include <cassert>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "arm_kinematics/joint_map/affine_joint_map.hpp"
#include "arm_kinematics/joint_map/composite_joint_map.hpp"
#include "arm_kinematics/joint_map/joint_map_blueprint.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_joint_map.hpp"
#include "arm_kinematics/joint_map/transmission_model.hpp"

namespace arm_kinematics {

using AnalysisStateInterfaceId = TransmissionAnalysis::StateInterfaceId;
using BlueprintStateInterfaceId = arm_kinematics::BlueprintStateInterfaceId;

namespace {

constexpr std::size_t kUnassignedSlot = std::numeric_limits<std::size_t>::max();

bool blueprint_is_pure_affine(const JointMapBlueprint & blueprint) noexcept
{
  for (const auto & seg : blueprint.segments()) {
    if (std::holds_alternative<JointMapBlueprintSegment::TransmissionStage>(seg.kind)) {
      return false;
    }
  }
  return true;
}

JointMap materialize_pure_affine(const JointMapBlueprint & blueprint)
{
  const auto inputs = blueprint.inputs();   // span<const StateInterfaceDefinition>
  const auto outputs = blueprint.outputs(); // span<const StateInterfaceDefinition>
  const auto state_interfaces = blueprint.state_interfaces();

  // First-occurrence-wins def → input slot for the user-facing input vector.
  std::unordered_map<StateInterfaceDefinition, std::size_t> bare_input_slot_of;
  bare_input_slot_of.reserve(inputs.size());
  std::vector<std::size_t> input_slot_of_blueprint_sid(state_interfaces.size(), kUnassignedSlot);

  for (const auto & seg : blueprint.segments()) {
    const auto * batch = std::get_if<JointMapBlueprintSegment::InputAffineBatch>(&seg.kind);
    assert(batch != nullptr && "materialize_pure_affine: non-affine segment in pure-affine blueprint");
  }

  for (std::size_t i = 0; i < inputs.size(); ++i) {
    bool assigned_registered = false;
    for (BlueprintStateInterfaceId local_id = 0; local_id < state_interfaces.size(); ++local_id) {
      if (state_interfaces[local_id].definition != inputs[i]) {
        continue;
      }
      if (input_slot_of_blueprint_sid[local_id] == kUnassignedSlot) {
        input_slot_of_blueprint_sid[local_id] = i;
      }
      assigned_registered = true;
    }
    if (!assigned_registered) {
      bare_input_slot_of.emplace(inputs[i], i);
    }
  }

  std::vector<std::size_t> sources(outputs.size(), 0);
  std::vector<double> multipliers(outputs.size(), 0.0);
  std::vector<double> offsets(outputs.size(), 0.0);
  std::vector<bool> filled(outputs.size(), false);

  for (const auto & seg : blueprint.segments()) {
    const auto * batch = std::get_if<JointMapBlueprintSegment::InputAffineBatch>(&seg.kind);
    assert(batch != nullptr && "materialize_pure_affine: non-affine segment in pure-affine blueprint");
    assert(batch->scratch_targets.empty() && "materialize_pure_affine: scratch-fill rows in pure-affine blueprint");
    for (std::size_t i = 0; i < batch->blueprint_output_indices.size(); ++i) {
      const std::size_t out_pos = batch->blueprint_output_indices[i];
      const BlueprintStateInterfaceId source_id = batch->sources[i];
      const auto & source = state_interfaces[source_id];
      if (input_slot_of_blueprint_sid[source_id] != kUnassignedSlot) {
        const std::size_t input_slot = input_slot_of_blueprint_sid[source_id];
        if (input_slot == kUnassignedSlot) {
          throw std::invalid_argument(
            "materialize_joint_map: pure-affine blueprint references a non-input source");
        }
        sources[out_pos] = input_slot;
      } else {
        const auto bare_it = bare_input_slot_of.find(source.definition);
        if (bare_it == bare_input_slot_of.end()) {
          throw std::invalid_argument(
            "materialize_joint_map: pure-affine blueprint references a non-input source");
        }
        sources[out_pos] = bare_it->second;
      }
      multipliers[out_pos] = batch->multipliers[i];
      offsets[out_pos] = batch->offsets[i];
      filled[out_pos] = true;
    }
  }

  for (std::size_t i = 0; i < outputs.size(); ++i) {
    if (!filled[i]) {
      throw std::invalid_argument(
        "materialize_joint_map: pure-affine blueprint left an output position with no producer");
    }
  }

  return JointMap(AffineJointMap(
    std::move(sources), std::move(multipliers), std::move(offsets), inputs.size()));
}

JointMap materialize_composite(
  const JointMapBlueprint & blueprint,
  const TransmissionAnalysis & analysis)
{
  const auto inputs = blueprint.inputs();   // span<const StateInterfaceDefinition>
  const auto outputs = blueprint.outputs(); // span<const StateInterfaceDefinition>
  const auto segments = blueprint.segments();
  const auto state_interfaces = blueprint.state_interfaces();
  const std::size_t blueprint_state_count = state_interfaces.size();

  // ---- Allocate scratch slots -----------------------------------------------
  // Scratch holds: (a) user inputs, (b) transmission outputs, (c) scratch-fill targets
  // (Case 3: affinely-derived defs needed as transmission inputs, neither inputs nor tx outputs).
  //
  // Affine batch output rows do NOT need scratch slots — the leaf-source invariant guarantees
  // no other segment reads them; they go directly to the composite's output buffer.
  std::vector<std::size_t> scratch_slot_of_blueprint_sid(blueprint_state_count, kUnassignedSlot);
  std::size_t next_scratch_slot = 0;

  auto allocate_blueprint_slot = [&](const BlueprintStateInterfaceId sid) -> std::size_t {
    if (scratch_slot_of_blueprint_sid[sid] == kUnassignedSlot) {
      scratch_slot_of_blueprint_sid[sid] = next_scratch_slot++;
    }
    return scratch_slot_of_blueprint_sid[sid];
  };

  // (a) Inputs (in order, first-occurrence-wins).
  for (BlueprintStateInterfaceId local_id = 0; local_id < blueprint_state_count; ++local_id) {
    const auto & record = state_interfaces[local_id];
    for (const auto & def : inputs) {
      if (record.definition == def) {
        allocate_blueprint_slot(local_id);
        break;
      }
    }
  }
  // (b) Transmission outputs.
  for (const auto & seg : segments) {
    if (const auto * tx = std::get_if<JointMapBlueprintSegment::TransmissionStage>(&seg.kind)) {
      for (const auto sid : tx->outputs) {
        allocate_blueprint_slot(sid);
      }
    }
  }
  // (c) Scratch-fill targets (Case 3 intermediate values).
  for (const auto & seg : segments) {
    if (const auto * batch = std::get_if<JointMapBlueprintSegment::InputAffineBatch>(&seg.kind)) {
      for (const auto target_id : batch->scratch_targets) {
        allocate_blueprint_slot(target_id);
      }
    }
  }

  const std::size_t scratch_size = next_scratch_slot;

  auto resolve_scratch = [&](const BlueprintStateInterfaceId local_id) -> std::size_t {
    const std::size_t slot = scratch_slot_of_blueprint_sid[local_id];
    if (slot == kUnassignedSlot) {
      throw std::invalid_argument(
        "materialize_joint_map: blueprint segment references a def with no scratch slot "
        "(violates leaf-source invariant or precondition)");
    }
    return slot;
  };

  // ---- Build input seeds ----------------------------------------------------
  std::vector<std::pair<std::size_t, std::size_t>> input_seeds;
  std::unordered_set<StateInterfaceDefinition> seeded;
  input_seeds.reserve(inputs.size());
  seeded.reserve(inputs.size());
  for (std::size_t i = 0; i < inputs.size(); ++i) {
    if (seeded.insert(inputs[i]).second) {
      for (BlueprintStateInterfaceId local_id = 0; local_id < blueprint_state_count; ++local_id) {
        if (state_interfaces[local_id].definition == inputs[i]) {
          input_seeds.emplace_back(i, resolve_scratch(local_id));
          break;
        }
      }
    }
  }

  // ---- Build stages ---------------------------------------------------------
  std::vector<CompositeJointMapStage> stages;
  stages.reserve(segments.size());

  for (const auto & seg : segments) {
    CompositeJointMapStage stage{};
    if (const auto * batch = std::get_if<JointMapBlueprintSegment::InputAffineBatch>(&seg.kind)) {
      const std::size_t out_count = batch->sources.size();
      const std::size_t fill_count = batch->scratch_targets.size();
      const std::size_t total_count = out_count + fill_count;

      // Build unified source/multiplier/offset arrays for the AffineJointMap.
      std::vector<std::size_t> aff_sources(total_count);
      std::vector<double> aff_multipliers(total_count);
      std::vector<double> aff_offsets(total_count);

      for (std::size_t i = 0; i < out_count; ++i) {
        aff_sources[i] = resolve_scratch(batch->sources[i]);
        aff_multipliers[i] = batch->multipliers[i];
        aff_offsets[i] = batch->offsets[i];
      }
      for (std::size_t i = 0; i < fill_count; ++i) {
        aff_sources[out_count + i] = resolve_scratch(batch->scratch_sources[i]);
        aff_multipliers[out_count + i] = batch->scratch_multipliers[i];
        aff_offsets[out_count + i] = batch->scratch_offsets[i];
      }

      AffineJointMap aff(
        std::move(aff_sources), std::move(aff_multipliers), std::move(aff_offsets), scratch_size);
      stage.segment = JointMap(std::move(aff));

      // Output rows → output_scatter; scratch-fill rows → scratch_scatter.
      stage.scratch_scatter.resize(total_count, std::nullopt);
      stage.output_scatter.resize(total_count, std::nullopt);

      for (std::size_t i = 0; i < out_count; ++i) {
        stage.output_scatter[i] = batch->blueprint_output_indices[i];
      }
      for (std::size_t i = 0; i < fill_count; ++i) {
        stage.scratch_scatter[out_count + i] = resolve_scratch(batch->scratch_targets[i]);
      }
    } else {
      const auto & tx = std::get<JointMapBlueprintSegment::TransmissionStage>(seg.kind);

      const auto & instance = analysis.transmissions().at(tx.instance_id);
      const auto & model = analysis.models().at(instance.model_id);
      if (!model) {
        throw std::invalid_argument(
          "materialize_joint_map: transmission model is null in the analysis");
      }
      std::vector<AnalysisStateInterfaceId> input_sids;
      input_sids.reserve(tx.inputs.size());
      for (const auto local_id : tx.inputs) {
        const auto opt_sid = state_interfaces[local_id].analysis_state_interface_id;
        if (!opt_sid.has_value()) {
          throw std::invalid_argument(
            "materialize_joint_map: transmission input missing analysis state interface id");
        }
        input_sids.push_back(*opt_sid);
      }

      std::vector<AnalysisStateInterfaceId> output_sids;
      output_sids.reserve(tx.outputs.size());
      for (const auto local_id : tx.outputs) {
        const auto opt_sid = state_interfaces[local_id].analysis_state_interface_id;
        if (!opt_sid.has_value()) {
          throw std::invalid_argument(
            "materialize_joint_map: transmission output missing analysis state interface id");
        }
        output_sids.push_back(*opt_sid);
      }

      auto compute = model->build(
        span<const AnalysisStateInterfaceId>(input_sids.data(), input_sids.size()),
        span<const AnalysisStateInterfaceId>(output_sids.data(), output_sids.size()));
      if (!compute) {
        throw std::invalid_argument(
          "materialize_joint_map: TransmissionModel::build returned a null compute");
      }

      // Gather: each transmission input SID maps to its scratch slot (pre-filled for Case 3).
      std::vector<std::size_t> gather(tx.inputs.size());
      for (std::size_t i = 0; i < tx.inputs.size(); ++i) {
        const std::size_t slot = scratch_slot_of_blueprint_sid[tx.inputs[i]];
        if (slot == kUnassignedSlot) {
          throw std::invalid_argument(
            "materialize_joint_map: transmission input has no scratch slot "
            "(violates leaf-source invariant or precondition)");
        }
        gather[i] = slot;
      }

      const std::size_t tx_output_count = tx.outputs.size();
      TransmissionJointMap tjm(
        std::move(compute),
        std::move(gather),
        tx_output_count,
        scratch_size);
      stage.segment = JointMap(std::move(tjm));

      stage.scratch_scatter.reserve(tx_output_count);
      stage.output_scatter.reserve(tx_output_count);
      for (std::size_t i = 0; i < tx_output_count; ++i) {
        const std::size_t slot = scratch_slot_of_blueprint_sid[tx.outputs[i]];
        if (slot == kUnassignedSlot) {
          throw std::invalid_argument(
            "materialize_joint_map: transmission output has no scratch slot "
            "(violates leaf-source invariant or precondition)");
        }
        stage.scratch_scatter.emplace_back(slot);
        stage.output_scatter.push_back(tx.blueprint_output_indices[i]);
      }
    }
    stages.push_back(std::move(stage));
  }

  std::vector<std::pair<std::size_t, std::size_t>> final_output_gather;

  return JointMap(CompositeJointMap(
    /*input_count=*/inputs.size(),
    /*output_count=*/outputs.size(),
    /*scratch_size=*/scratch_size,
    std::move(input_seeds),
    std::move(stages),
    std::move(final_output_gather)));
}

}  // namespace

JointMap materialize_joint_map(
  const JointMapBlueprint & blueprint,
  const TransmissionAnalysis & analysis)
{
  if (blueprint_is_pure_affine(blueprint)) {
    return materialize_pure_affine(blueprint);
  }
  return materialize_composite(blueprint, analysis);
}

}  // namespace arm_kinematics
