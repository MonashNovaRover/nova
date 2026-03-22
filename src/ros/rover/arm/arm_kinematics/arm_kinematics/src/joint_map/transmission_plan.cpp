//
// Created by Bailey Chessum on 24/03/2026.
//

#include "arm_kinematics/joint_map/transmission_plan.hpp"

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

} // namespace

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
