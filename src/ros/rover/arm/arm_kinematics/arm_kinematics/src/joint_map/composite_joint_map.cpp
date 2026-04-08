//
// Created by Bailey Chessum on 22/03/2026.
//

#include "arm_kinematics/joint_map/composite_joint_map.hpp"

#include <algorithm>
#include <stdexcept>

namespace arm_kinematics {

CompositeJointMap::CompositeJointMap(std::vector<CompositeJointMapSegment> segments)
  : segments_(std::move(segments))
{
  if (segments_.empty()) {
    return;
  }

  input_count_ = segments_.front().joint_map.input_count();
  std::vector<bool> output_index_seen{};
  for (const auto & segment : segments_) {
    if (!segment.joint_map.valid()) {
      throw std::invalid_argument("CompositeJointMap received an invalid JointMap segment");
    }
    if (segment.joint_map.input_count() != input_count_) {
      throw std::invalid_argument("CompositeJointMap segment input counts must all match");
    }
    if (segment.joint_map.output_count() != segment.output_indices.size()) {
      throw std::invalid_argument(
        "CompositeJointMap segment output indices must match the segment JointMap output count");
    }

    for (const auto output_index : segment.output_indices) {
      output_count_ = std::max(output_count_, output_index + 1);
    }
  }

  output_index_seen.assign(output_count_, false);
  for (const auto & segment : segments_) {
    for (const auto output_index : segment.output_indices) {
      if (output_index_seen[output_index]) {
        throw std::invalid_argument("CompositeJointMap received duplicate output indices across segments");
      }
      output_index_seen[output_index] = true;
    }
  }

  workspace_ = make_workspace();
}

CompositeJointMap::Workspace CompositeJointMap::make_workspace() const
{
  Workspace workspace{};
  workspace.segment_outputs.reserve(segments_.size());

  for (const auto & segment : segments_) {
    workspace.segment_outputs.emplace_back(segment.joint_map.output_count(), 0.0F);
  }

  return workspace;
}

void CompositeJointMap::map(const span<const float> inputs, const span<float> outputs) const
{
  if (inputs.size() != input_count_) {
    throw std::invalid_argument("CompositeJointMap::map() received inputs with the wrong size");
  }
  if (outputs.size() != output_count_) {
    throw std::invalid_argument("CompositeJointMap::map() received outputs with the wrong size");
  }

  for (size_t i = 0; i < segments_.size(); ++i) {
    auto & segment_outputs = workspace_.segment_outputs[i];
    segments_[i].joint_map.map(inputs, segment_outputs);

    for (size_t output_i = 0; output_i < segments_[i].output_indices.size(); ++output_i) {
      outputs[segments_[i].output_indices[output_i]] = segment_outputs[output_i];
    }
  }
}

StagedJointMap::StagedJointMap(std::vector<JointMap> stages)
  : stages_(std::move(stages))
{
  if (stages_.empty()) {
    return;
  }

  input_count_ = stages_.front().input_count();
  output_count_ = stages_.back().output_count();

  for (size_t i = 0; i < stages_.size(); ++i) {
    if (!stages_[i].valid()) {
      throw std::invalid_argument("StagedJointMap received an invalid stage");
    }

    if (i + 1 < stages_.size() && stages_[i].output_count() != stages_[i + 1].input_count()) {
      throw std::invalid_argument("StagedJointMap stages must chain through matching output/input counts");
    }
  }

  workspace_ = make_workspace();
}

StagedJointMap::Workspace StagedJointMap::make_workspace() const
{
  Workspace workspace{};
  if (!stages_.empty()) {
    workspace.stage_inputs.reserve(stages_.size() - 1);
  }
  workspace.stage_outputs.reserve(stages_.size());
  for (size_t i = 0; i < stages_.size(); ++i) {
    const auto & stage = stages_[i];
    workspace.stage_outputs.emplace_back(stage.output_count(), 0.0F);
    if (i + 1 < stages_.size()) {
    workspace.stage_inputs.emplace_back(stages_[i + 1].input_count(), 0.0F);
    }
  }
  return workspace;
}

void StagedJointMap::map(const span<const float> inputs, const span<float> outputs) const
{
  if (inputs.size() != input_count_) {
    throw std::invalid_argument("StagedJointMap::map() received inputs with the wrong size");
  }
  if (outputs.size() != output_count_) {
    throw std::invalid_argument("StagedJointMap::map() received outputs with the wrong size");
  }

  if (stages_.empty()) {
    return;
  }

  stages_.front().map(inputs, workspace_.stage_outputs.front());
  for (size_t i = 1; i < stages_.size(); ++i) {
    auto & stage_inputs = workspace_.stage_inputs[i - 1];
    const auto & previous_outputs = workspace_.stage_outputs[i - 1];
    for (size_t j = 0; j < previous_outputs.size(); ++j) {
      stage_inputs[j] = previous_outputs[j];
    }

    stages_[i].map(
      span<const float>(stage_inputs.data(), stage_inputs.size()),
      workspace_.stage_outputs[i]);
  }

  const auto & final_outputs = workspace_.stage_outputs.back();
  std::copy(final_outputs.begin(), final_outputs.end(), outputs.begin());
}

// NOTE: The legacy `compile_joint_map_plan_expected` and `compile_joint_map_plan_stage_expected`
// helpers have been removed in step 2 of the state-interface refactor. The new direct
// construction path that walks a TransmissionSubgraph will be added in step 5 / step 6.

} // namespace arm_kinematics
