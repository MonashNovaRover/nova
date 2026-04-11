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
  const auto inputs = blueprint.inputs();
  const auto outputs = blueprint.outputs();

  // First-occurrence-wins SID → input slot for the user-facing input vector.
  std::unordered_map<StateInterfaceId, std::size_t> input_slot_of;
  for (std::size_t i = 0; i < inputs.size(); ++i) {
    input_slot_of.emplace(inputs[i], i);
  }

  // Pre-size for output count; we'll fill in row-by-row from segments.
  std::vector<std::size_t> sources(outputs.size(), 0);
  std::vector<double> multipliers(outputs.size(), 0.0);
  std::vector<double> offsets(outputs.size(), 0.0);
  std::vector<bool> filled(outputs.size(), false);

  for (const auto & seg : blueprint.segments()) {
    const auto * batch = std::get_if<JointMapBlueprintSegment::InputAffineBatch>(&seg.kind);
    // Defensive: this is an internal invariant — `materialize_pure_affine` is only called when
    // `blueprint_is_pure_affine(blueprint)` is true, which already excludes any non-affine
    // segments. If we hit this, the materializer's classification logic is buggy.
    assert(batch != nullptr && "materialize_pure_affine: non-affine segment in pure-affine blueprint");
    for (std::size_t i = 0; i < batch->blueprint_output_indices.size(); ++i) {
      const std::size_t out_pos = batch->blueprint_output_indices[i];
      const auto src_sid = batch->sources[i];
      const auto it = input_slot_of.find(src_sid);
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
  const auto inputs = blueprint.inputs();
  const auto outputs = blueprint.outputs();
  const auto segments = blueprint.segments();

  // ---- Allocate scratch slots -----------------------------------------------
  // We need a scratch slot for every distinct StateInterfaceId that needs to live in the
  // value table at any point during execution: (a) inputs, (b) transmission outputs (so
  // downstream consumers can read them).
  //
  // Affine batch outputs do NOT need scratch slots — by the leaf-source invariant, no other
  // segment reads them, so they go directly to the composite's output buffer.
  //
  // Affine batch sources are always either inputs or transmission outputs (per the
  // leaf-source invariant), both of which already have slots from the above.
  std::unordered_map<StateInterfaceId, std::size_t> scratch_slot_of;
  auto allocate_slot = [&](const StateInterfaceId sid) -> std::size_t {
    const auto [it, inserted] = scratch_slot_of.emplace(sid, scratch_slot_of.size());
    return it->second;
  };

  // (a) Inputs (in order, first-occurrence-wins).
  for (const auto sid : inputs) {
    allocate_slot(sid);
  }
  // (b) Transmission outputs.
  for (const auto & seg : segments) {
    if (const auto * tx = std::get_if<JointMapBlueprintSegment::TransmissionStage>(&seg.kind)) {
      for (const auto sid : tx->outputs) {
        allocate_slot(sid);
      }
    }
  }

  const std::size_t scratch_size = scratch_slot_of.size();

  auto resolve_scratch = [&](const StateInterfaceId sid) -> std::size_t {
    const auto it = scratch_slot_of.find(sid);
    if (it == scratch_slot_of.end()) {
      throw std::invalid_argument(
        "materialize_joint_map: blueprint segment references a SID with no scratch slot "
        "(violates leaf-source invariant or precondition)");
    }
    return it->second;
  };

  // ---- Build input seeds ----------------------------------------------------
  // For each input slot in `inputs`, copy that input value into its scratch slot. Duplicates
  // in `inputs` are skipped — first-occurrence wins (matches blueprint semantics).
  std::vector<std::pair<std::size_t, std::size_t>> input_seeds;
  std::unordered_set<StateInterfaceId> seeded;
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
      // Build an AffineJointMap whose `input_count` is the composite's scratch size and whose
      // `sources` index scratch slots.
      std::vector<std::size_t> sources(batch->sources.size());
      for (std::size_t i = 0; i < batch->sources.size(); ++i) {
        sources[i] = resolve_scratch(batch->sources[i]);
      }
      AffineJointMap aff(
        std::move(sources),
        batch->multipliers,
        batch->offsets,
        scratch_size);
      stage.segment = JointMap(std::move(aff));

      // Each affine row writes its result directly to a final-output slot. None go to scratch
      // (no downstream consumer needs them — leaf-source invariant).
      const std::size_t row_count = batch->blueprint_output_indices.size();
      stage.scratch_scatter.assign(row_count, std::nullopt);
      stage.output_scatter.reserve(row_count);
      for (const std::size_t out_pos : batch->blueprint_output_indices) {
        stage.output_scatter.emplace_back(out_pos);
      }
    } else {
      const auto & tx = std::get<JointMapBlueprintSegment::TransmissionStage>(seg.kind);

      // Build the ComputeTransmission via the analysis's TransmissionModel.
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

      // Gather indices: each transmission input SID maps to its scratch slot.
      std::vector<std::size_t> gather(tx.inputs.size());
      for (std::size_t i = 0; i < tx.inputs.size(); ++i) {
        gather[i] = resolve_scratch(tx.inputs[i]);
      }

      const std::size_t tx_output_count = tx.outputs.size();
      TransmissionJointMap tjm(
        std::move(compute),
        std::move(gather),
        tx_output_count,
        scratch_size);
      stage.segment = JointMap(std::move(tjm));

      // For each transmission output: scatter to scratch (downstream consumers may read it),
      // and additionally scatter to the final output buffer if it's a directly-wanted output.
      stage.scratch_scatter.reserve(tx_output_count);
      stage.output_scatter.reserve(tx_output_count);
      for (std::size_t i = 0; i < tx_output_count; ++i) {
        stage.scratch_scatter.emplace_back(resolve_scratch(tx.outputs[i]));
        stage.output_scatter.push_back(tx.blueprint_output_indices[i]);
      }
    }
    stages.push_back(std::move(stage));
  }

  // ---- Final output gather --------------------------------------------------
  // Currently empty: every wanted output is written by a stage (either an affine row or a
  // transmission scatter). The final-gather mechanism is exposed for forward compatibility
  // (e.g., a future blueprint that emits direct passthroughs without a wrapping stage).
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
