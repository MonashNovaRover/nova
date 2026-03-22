//
// Created by Bailey Chessum on 22/03/2026.
//

#include "arm_kinematics/joint_map/composite_joint_map.hpp"

#include "arm_kinematics/joint_map/affine_joint_map.hpp"
#include "arm_kinematics/joint_map/transmission_joint_map.hpp"

#include <algorithm>
#include <stdexcept>

namespace arm_kinematics {

namespace {

bool has_identity_output_indices(const std::vector<size_t> & output_indices)
{
  for (size_t i = 0; i < output_indices.size(); ++i) {
    if (output_indices[i] != i) {
      return false;
    }
  }
  return true;
}

} // namespace

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

void CompositeJointMap::map(const span<const double> inputs, const span<float> outputs) const
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

CompileJointMapPlanResult compile_joint_map_plan_expected(
  const TransmissionAnalysis & analysis,
  const JointMapPlan & joint_map_plan,
  const JointQuantity quantity)
{
  if (joint_map_plan.segments.empty()) {
    return tl::make_unexpected("Joint map plan does not contain any executable segments");
  }

  if (joint_map_plan.segments.size() == 1) {
    const auto & segment = joint_map_plan.segments.front();
    if (has_identity_output_indices(segment.output_indices)) {
      if (const auto * affine_plan = std::get_if<AffinePlan>(&segment.plan)) {
        return JointMap(AffineJointMap(*affine_plan));
      }

      const auto & transmission_plan = std::get<TransmissionPlan>(segment.plan);
      auto compiled_transmission_plan = compile_transmission_plan_expected(analysis, transmission_plan, quantity);
      if (!compiled_transmission_plan.has_value()) {
        return tl::make_unexpected(compiled_transmission_plan.error());
      }
      return JointMap(TransmissionJointMap(std::move(*compiled_transmission_plan)));
    }
  }

  std::vector<CompositeJointMapSegment> segments{};
  segments.reserve(joint_map_plan.segments.size());

  for (const auto & segment : joint_map_plan.segments) {
    if (const auto * affine_plan = std::get_if<AffinePlan>(&segment.plan)) {
      segments.push_back(CompositeJointMapSegment{
        JointMap(AffineJointMap(*affine_plan)),
        segment.output_indices
      });
      continue;
    }

    const auto & transmission_plan = std::get<TransmissionPlan>(segment.plan);
    auto compiled_transmission_plan = compile_transmission_plan_expected(analysis, transmission_plan, quantity);
    if (!compiled_transmission_plan.has_value()) {
      return tl::make_unexpected(compiled_transmission_plan.error());
    }

    segments.push_back(CompositeJointMapSegment{
      JointMap(TransmissionJointMap(std::move(*compiled_transmission_plan))),
      segment.output_indices
    });
  }

  return JointMap(CompositeJointMap(std::move(segments)));
}

} // namespace arm_kinematics
