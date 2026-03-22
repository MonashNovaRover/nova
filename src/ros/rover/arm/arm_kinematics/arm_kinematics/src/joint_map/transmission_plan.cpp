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

const std::vector<JointId> & consumed_joint_ids_for_direction(
  const TransmissionAnalysis::TransmissionInstance & transmission,
  const PropagationDirection direction)
{
  return direction == PropagationDirection::Forward ?
    transmission.input_joint_ids :
    transmission.output_joint_ids;
}

const std::vector<JointId> & produced_joint_ids_for_direction(
  const TransmissionAnalysis::TransmissionInstance & transmission,
  const PropagationDirection direction)
{
  return direction == PropagationDirection::Forward ?
    transmission.output_joint_ids :
    transmission.input_joint_ids;
}

std::optional<TransmissionPlanStage> make_direct_stage(
  const TransmissionAnalysis::TransmissionInstance & transmission,
  const TransmissionGroupId group_id,
  const TransmissionModel & model,
  const PropagationDirection direction,
  span<const JointId> requested_inputs,
  span<const JointId> requested_outputs,
  const JointQuantity quantity)
{
  const auto & consumed_joint_ids = consumed_joint_ids_for_direction(transmission, direction);
  const auto & produced_joint_ids = produced_joint_ids_for_direction(transmission, direction);

  if (!model.can_build(quantity, direction)) {
    return std::nullopt;
  }
  if (consumed_joint_ids != std::vector<JointId>(requested_inputs.begin(), requested_inputs.end())) {
    return std::nullopt;
  }
  if (produced_joint_ids != std::vector<JointId>(requested_outputs.begin(), requested_outputs.end())) {
    return std::nullopt;
  }

  return TransmissionPlanStage{
    group_id,
    direction,
    consumed_joint_ids,
    produced_joint_ids
  };
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
  if (affine_it == affine_transmissions.end() && input_joint_ids.empty()) {
    return AffinePlanStage{
      output_joint_id,
      output_joint_id,
      0,
      0.0F,
      0.0F
    };
  }
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

  std::optional<TransmissionPlan> direct_plan = std::nullopt;

  for (TransmissionGroupId group_id = 0; group_id < transmissions.size(); ++group_id) {
    const auto & transmission = transmissions[group_id];
    const auto & model = *models[transmission.model_id];

    for (const auto direction : {PropagationDirection::Forward, PropagationDirection::Reverse}) {
      const auto direct_stage = make_direct_stage(
        transmission,
        group_id,
        model,
        direction,
        input_joint_ids,
        output_joint_ids,
        quantity);
      if (!direct_stage.has_value()) {
        continue;
      }

      if (direct_plan.has_value()) {
        return tl::make_unexpected("Ambiguous transmission plan: multiple direct candidates were found");
      }

      direct_plan = TransmissionPlan{
        {input_joint_ids.begin(), input_joint_ids.end()},
        {output_joint_ids.begin(), output_joint_ids.end()},
        {*direct_stage}
      };
    }
  }

  if (direct_plan.has_value()) {
    return *direct_plan;
  }

  std::optional<TransmissionPlan> two_stage_plan = std::nullopt;

  for (TransmissionGroupId first_group_id = 0; first_group_id < transmissions.size(); ++first_group_id) {
    const auto & first_transmission = transmissions[first_group_id];
    const auto & first_model = *models[first_transmission.model_id];

    for (const auto first_direction : {PropagationDirection::Forward, PropagationDirection::Reverse}) {
      const auto & intermediate_joint_ids =
        produced_joint_ids_for_direction(first_transmission, first_direction);
      const auto first_stage = make_direct_stage(
        first_transmission,
        first_group_id,
        first_model,
        first_direction,
        input_joint_ids,
        span<const JointId>(intermediate_joint_ids),
        quantity);
      if (!first_stage.has_value()) {
        continue;
      }

      for (TransmissionGroupId second_group_id = 0; second_group_id < transmissions.size(); ++second_group_id) {
        const auto & second_transmission = transmissions[second_group_id];
        const auto & second_model = *models[second_transmission.model_id];

        for (const auto second_direction : {PropagationDirection::Forward, PropagationDirection::Reverse}) {
          const auto second_stage = make_direct_stage(
            second_transmission,
            second_group_id,
            second_model,
            second_direction,
            span<const JointId>(intermediate_joint_ids),
            output_joint_ids,
            quantity);
          if (!second_stage.has_value()) {
            continue;
          }

          if (two_stage_plan.has_value()) {
            return tl::make_unexpected("Ambiguous transmission plan: multiple two-stage candidates were found");
          }

          two_stage_plan = TransmissionPlan{
            {input_joint_ids.begin(), input_joint_ids.end()},
            {output_joint_ids.begin(), output_joint_ids.end()},
            {*first_stage, *second_stage}
          };
        }
      }
    }
  }

  if (!two_stage_plan.has_value()) {
    return tl::make_unexpected(
      "No direct or two-stage transmission plan found for inputs [" + join_joint_ids(input_joint_ids) +
      "] and outputs [" + join_joint_ids(output_joint_ids) + "]");
  }

  return *two_stage_plan;
}

} // namespace arm_kinematics
