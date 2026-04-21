//
// Created by Bailey Chessum on 22/03/2026.
//

#include "arm_kinematics/joint_map/composite_joint_map.hpp"

#include <cassert>
#include <stdexcept>
#include <utility>

namespace arm_kinematics {

CompositeJointMap::CompositeJointMap(
  const size_t input_count,
  const size_t output_count,
  const size_t scratch_size,
  std::vector<std::pair<size_t, size_t>> input_seeds,
  std::vector<CompositeJointMapStage> stages,
  std::vector<std::pair<size_t, size_t>> final_output_gather)
  : input_count_(input_count),
    output_count_(output_count),
    scratch_size_(scratch_size),
    input_seeds_(std::move(input_seeds)),
    stages_(std::move(stages)),
    final_output_gather_(std::move(final_output_gather))
{
  // Validate input seeds.
  for (const auto & [in_slot, scratch_slot] : input_seeds_) {
    if (in_slot >= input_count_) {
      throw std::invalid_argument(
        "CompositeJointMap: input_seeds entry has input_slot out of range");
    }
    if (scratch_slot >= scratch_size_) {
      throw std::invalid_argument(
        "CompositeJointMap: input_seeds entry has scratch_slot out of range");
    }
  }

  // Validate stages.
  for (const auto & stage : stages_) {
    if (!stage.segment.valid()) {
      throw std::invalid_argument("CompositeJointMap: stage has an invalid segment");
    }
    if (stage.segment.input_count() != scratch_size_) {
      throw std::invalid_argument(
        "CompositeJointMap: stage segment input_count must equal the composite's scratch_size");
    }
    const size_t out_count = stage.segment.output_count();
    if (stage.scratch_scatter.size() != out_count ||
        stage.output_scatter.size() != out_count)
    {
      throw std::invalid_argument(
        "CompositeJointMap: stage scratter vectors must match the segment output count");
    }
    for (const auto & opt_slot : stage.scratch_scatter) {
      if (opt_slot.has_value() && *opt_slot >= scratch_size_) {
        throw std::invalid_argument(
          "CompositeJointMap: stage scratch_scatter slot out of range");
      }
    }
    for (const auto & opt_slot : stage.output_scatter) {
      if (opt_slot.has_value() && *opt_slot >= output_count_) {
        throw std::invalid_argument(
          "CompositeJointMap: stage output_scatter slot out of range");
      }
    }
  }

  // Validate final output gather.
  for (const auto & [scratch_slot, out_slot] : final_output_gather_) {
    if (scratch_slot >= scratch_size_) {
      throw std::invalid_argument(
        "CompositeJointMap: final_output_gather entry has scratch_slot out of range");
    }
    if (out_slot >= output_count_) {
      throw std::invalid_argument(
        "CompositeJointMap: final_output_gather entry has output_slot out of range");
    }
  }

  workspace_ = make_workspace();
}

CompositeJointMap::Workspace CompositeJointMap::make_workspace() const
{
  Workspace ws{};
  ws.scratch.assign(scratch_size_, 0.0);
  ws.stage_outputs.reserve(stages_.size());
  for (const auto & stage : stages_) {
    ws.stage_outputs.emplace_back(stage.segment.output_count(), 0.0);
  }
  return ws;
}

void CompositeJointMap::map(const span<const double> inputs, const span<double> outputs) const
{
  assert(inputs.size() == input_count_);
  assert(outputs.size() == output_count_);

  // Phase 1: seed scratch from inputs.
  for (const auto & [in_slot, scratch_slot] : input_seeds_) {
    workspace_.scratch[scratch_slot] = inputs[in_slot];
  }

  // Phase 2: run each stage and scatter its outputs.
  for (size_t i = 0; i < stages_.size(); ++i) {
    const auto & stage = stages_[i];
    auto & stage_out = workspace_.stage_outputs[i];
    stage.segment.map(
      span<const double>(workspace_.scratch.data(), workspace_.scratch.size()),
      span<double>(stage_out.data(), stage_out.size()));

    for (size_t j = 0; j < stage_out.size(); ++j) {
      if (const auto & slot = stage.scratch_scatter[j]; slot.has_value()) {
        workspace_.scratch[*slot] = stage_out[j];
      }
      if (const auto & slot = stage.output_scatter[j]; slot.has_value()) {
        outputs[*slot] = stage_out[j];
      }
    }
  }

  // Phase 3: final gather (used for direct passthroughs and similar).
  for (const auto & [scratch_slot, out_slot] : final_output_gather_) {
    outputs[out_slot] = workspace_.scratch[scratch_slot];
  }
}

} // namespace arm_kinematics
