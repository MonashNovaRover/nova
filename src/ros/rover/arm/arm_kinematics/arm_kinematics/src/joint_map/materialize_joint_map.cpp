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

namespace {

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

  // First-occurrence-wins def → input slot for the user-facing input vector.
  std::unordered_map<StateInterfaceDefinition, std::size_t> input_slot_of;
  for (std::size_t i = 0; i < inputs.size(); ++i) {
    input_slot_of.emplace(inputs[i], i);
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
      const auto src_def = batch->sources[i];
      const auto it = input_slot_of.find(src_def);
      if (it == input_slot_of.end()) {
        throw std::invalid_argument(
          "materialize_joint_map: pure-affine blueprint references a non-input source");
      }
      sources[out_pos] = it->second;
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

  // ---- Allocate scratch slots -----------------------------------------------
  // Scratch holds: (a) user inputs, (b) transmission outputs, (c) scratch-fill targets
  // (Case 3: affinely-derived defs needed as transmission inputs, neither inputs nor tx outputs).
  //
  // Affine batch output rows do NOT need scratch slots — the leaf-source invariant guarantees
  // no other segment reads them; they go directly to the composite's output buffer.
  std::unordered_map<StateInterfaceDefinition, std::size_t> scratch_slot_of;
  auto allocate_slot = [&](const StateInterfaceDefinition & def) -> std::size_t {
    const auto [it, inserted] = scratch_slot_of.emplace(def, scratch_slot_of.size());
    return it->second;
  };

  // (a) Inputs (in order, first-occurrence-wins).
  for (const auto & def : inputs) {
    allocate_slot(def);
  }
  // (b) Transmission outputs.
  for (const auto & seg : segments) {
    if (const auto * tx = std::get_if<JointMapBlueprintSegment::TransmissionStage>(&seg.kind)) {
      for (const auto sid : tx->outputs) {
        allocate_slot(analysis.state_interface_order().inverse[sid]);
      }
    }
  }
  // (c) Scratch-fill targets (Case 3 intermediate values).
  for (const auto & seg : segments) {
    if (const auto * batch = std::get_if<JointMapBlueprintSegment::InputAffineBatch>(&seg.kind)) {
      for (const auto & target_def : batch->scratch_targets) {
        allocate_slot(target_def);
      }
    }
  }

  const std::size_t scratch_size = scratch_slot_of.size();

  auto resolve_scratch = [&](const StateInterfaceDefinition & def) -> std::size_t {
    const auto it = scratch_slot_of.find(def);
    if (it == scratch_slot_of.end()) {
      throw std::invalid_argument(
        "materialize_joint_map: blueprint segment references a def with no scratch slot "
        "(violates leaf-source invariant or precondition)");
    }
    return it->second;
  };

  // ---- Build input seeds ----------------------------------------------------
  std::vector<std::pair<std::size_t, std::size_t>> input_seeds;
  std::unordered_set<StateInterfaceDefinition> seeded;
  input_seeds.reserve(inputs.size());
  for (std::size_t i = 0; i < inputs.size(); ++i) {
    if (seeded.insert(inputs[i]).second) {
      input_seeds.emplace_back(i, scratch_slot_of[inputs[i]]);
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
      auto compute = model->build(
        span<const StateInterfaceId>(tx.inputs.data(), tx.inputs.size()),
        span<const StateInterfaceId>(tx.outputs.data(), tx.outputs.size()));
      if (!compute) {
        throw std::invalid_argument(
          "materialize_joint_map: TransmissionModel::build returned a null compute");
      }

      // Gather: each transmission input SID maps to its scratch slot (pre-filled for Case 3).
      std::vector<std::size_t> gather(tx.inputs.size());
      for (std::size_t i = 0; i < tx.inputs.size(); ++i) {
        gather[i] = resolve_scratch(analysis.state_interface_order().inverse[tx.inputs[i]]);
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
        stage.scratch_scatter.emplace_back(
          resolve_scratch(analysis.state_interface_order().inverse[tx.outputs[i]]));
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
