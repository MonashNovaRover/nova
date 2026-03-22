//
// Created by Bailey Chessum on 24/03/2026.
//

#include "arm_kinematics/joint_map/transmission_plan.hpp"

#include <algorithm>
#include <optional>
#include <sstream>

namespace arm_kinematics {

namespace {

std::string join_joint_ids(span<const JointId> joint_ids)
{
  std::ostringstream ss;
  auto first = true;
  for (const auto joint_id : joint_ids) {
    if (!first)
      ss << ", ";
    ss << joint_id;
    first = false;
  }
  return ss.str();
}

tl::expected<AffinePlanStage, std::string> make_affine_plan_stage_expected(
  const TransmissionAnalysis & analysis,
  const span<const JointId> input_joint_ids,
  const JointId output_joint_id)
{
  const auto input_it = std::find(input_joint_ids.begin(), input_joint_ids.end(), output_joint_id);
  if (input_it != input_joint_ids.end()) {
    return AffinePlanStage{
      output_joint_id,
      output_joint_id,
      static_cast<size_t>(input_it - input_joint_ids.begin()),
      1.0F,
      0.0F
    };
  }

  const auto & affine_transmissions = analysis.affine_transmissions();
  const auto affine_it = std::find_if(
    affine_transmissions.begin(),
    affine_transmissions.end(),
    [output_joint_id](const TransmissionAnalysis::AffineTransmission & affine_transmission) {
      return affine_transmission.target_joint_id == output_joint_id;
    });
  if (affine_it == affine_transmissions.end()) {
    return tl::make_unexpected("No affine plan found for output joint " + std::to_string(output_joint_id));
  }

  const auto duplicate_affine_it = std::find_if(
    std::next(affine_it),
    affine_transmissions.end(),
    [output_joint_id](const TransmissionAnalysis::AffineTransmission & affine_transmission) {
      return affine_transmission.target_joint_id == output_joint_id;
    });
  if (duplicate_affine_it != affine_transmissions.end()) {
    return tl::make_unexpected(
      "Ambiguous affine plan: multiple affine transmissions target joint " + std::to_string(output_joint_id));
  }

  const auto source_stage = make_affine_plan_stage_expected(analysis, input_joint_ids, affine_it->source_joint_id);
  if (!source_stage.has_value()) {
    return tl::make_unexpected(source_stage.error());
  }

  return AffinePlanStage{
    source_stage->source_joint_id,
    output_joint_id,
    source_stage->source_input_index,
    source_stage->multiplier * affine_it->multiplier,
    source_stage->offset * affine_it->multiplier + affine_it->offset
  };
}

} // namespace

MakeAffinePlanResult make_affine_plan_expected(
  const TransmissionAnalysis & analysis,
  const span<const JointId> input_joint_ids,
  const span<const JointId> output_joint_ids)
{
  AffinePlan plan{
    {input_joint_ids.begin(), input_joint_ids.end()},
    {output_joint_ids.begin(), output_joint_ids.end()},
    {}
  };
  plan.stages.reserve(output_joint_ids.size());

  for (const auto output_joint_id : output_joint_ids) {
    const auto stage = make_affine_plan_stage_expected(analysis, input_joint_ids, output_joint_id);
    if (!stage.has_value()) {
      return tl::make_unexpected(
        "No affine plan found for inputs [" + join_joint_ids(input_joint_ids) +
        "] and outputs [" + join_joint_ids(output_joint_ids) + "]: " + stage.error());
    }

    plan.stages.push_back(*stage);
  }

  return plan;
}

MakeTransmissionPlanResult make_transmission_plan_expected(
  const TransmissionAnalysis & analysis,
  const span<const JointId> input_joint_ids,
  const span<const JointId> output_joint_ids,
  const JointQuantity quantity)
{
  const auto & transmissions = analysis.transmissions();
  const auto & models = analysis.models();

  std::optional<TransmissionPlan> plan = std::nullopt;

  for (TransmissionGroupId group_id = 0; group_id < transmissions.size(); ++group_id) {
    const auto & transmission = transmissions[group_id];
    const auto & model = *models[transmission.model_id];

    const bool direct_forward =
      transmission.input_joint_ids == std::vector<JointId>(input_joint_ids.begin(), input_joint_ids.end()) &&
      transmission.output_joint_ids == std::vector<JointId>(output_joint_ids.begin(), output_joint_ids.end());

    if (direct_forward && model.can_build(quantity, PropagationDirection::Forward)) {
      if (plan.has_value()) {
        return tl::make_unexpected("Ambiguous transmission plan: multiple direct forward candidates were found");
      }

      plan = TransmissionPlan{
        {input_joint_ids.begin(), input_joint_ids.end()},
        {output_joint_ids.begin(), output_joint_ids.end()},
        {TransmissionPlanStage{
          group_id,
          PropagationDirection::Forward,
          {input_joint_ids.begin(), input_joint_ids.end()},
          {output_joint_ids.begin(), output_joint_ids.end()}
        }}
      };
    }

    const bool direct_reverse =
      transmission.output_joint_ids == std::vector<JointId>(input_joint_ids.begin(), input_joint_ids.end()) &&
      transmission.input_joint_ids == std::vector<JointId>(output_joint_ids.begin(), output_joint_ids.end());

    if (direct_reverse && model.can_build(quantity, PropagationDirection::Reverse)) {
      if (plan.has_value()) {
        return tl::make_unexpected("Ambiguous transmission plan: multiple direct reverse candidates were found");
      }

      plan = TransmissionPlan{
        {input_joint_ids.begin(), input_joint_ids.end()},
        {output_joint_ids.begin(), output_joint_ids.end()},
        {TransmissionPlanStage{
          group_id,
          PropagationDirection::Reverse,
          {input_joint_ids.begin(), input_joint_ids.end()},
          {output_joint_ids.begin(), output_joint_ids.end()}
        }}
      };
    }
  }

  if (!plan.has_value()) {
    return tl::make_unexpected(
      "No direct transmission plan found for inputs [" + join_joint_ids(input_joint_ids) +
      "] and outputs [" + join_joint_ids(output_joint_ids) + "]");
  }

  return *plan;
}

} // namespace arm_kinematics
