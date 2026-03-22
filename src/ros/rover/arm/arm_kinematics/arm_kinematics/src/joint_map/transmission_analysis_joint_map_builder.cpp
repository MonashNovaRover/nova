//
// Created by Bailey Chessum on 22/03/2026.
//

#include "arm_kinematics/joint_map/transmission_analysis_joint_map_builder.hpp"

#include "arm_kinematics/joint_map/composite_joint_map.hpp"

#include <stdexcept>

namespace arm_kinematics {

namespace {

struct ResolvedOutputJointIds {
  std::vector<JointId> known_joint_ids{};
  std::vector<size_t> unknown_output_indices{};
};

std::vector<JointId> to_joint_ids_expected(
  const Order<std::string, JointId> & joint_order,
  const std::vector<std::string> & joint_names,
  const char * label)
{
  std::vector<JointId> joint_ids{};
  joint_ids.reserve(joint_names.size());

  for (const auto & joint_name : joint_names) {
    if (!joint_order.contains_key(joint_name)) {
      throw std::runtime_error(
        std::string("No canonical JointId found for ") + label + " joint '" + joint_name + "'");
    }

    joint_ids.push_back(joint_order[joint_name]);
  }

  return joint_ids;
}

ResolvedOutputJointIds resolve_output_joint_ids(
  const Order<std::string, JointId> & joint_order,
  const std::vector<std::string> & output_names)
{
  ResolvedOutputJointIds resolved{};
  resolved.known_joint_ids.reserve(output_names.size());
  resolved.unknown_output_indices.reserve(output_names.size());

  for (size_t i = 0; i < output_names.size(); ++i) {
    const auto & output_name = output_names[i];
    if (!joint_order.contains_key(output_name)) {
      resolved.unknown_output_indices.push_back(i);
      continue;
    }

    resolved.known_joint_ids.push_back(joint_order[output_name]);
  }

  return resolved;
}

JointMapPlanSegment make_zero_affine_segment(
  const std::vector<JointId> & stage_input_joint_ids,
  std::vector<size_t> output_indices)
{
  AffinePlan zero_plan{};
  zero_plan.input_joint_ids = stage_input_joint_ids;
  zero_plan.output_joint_ids.resize(output_indices.size(), 0);
  zero_plan.stages.reserve(output_indices.size());

  for (size_t i = 0; i < output_indices.size(); ++i) {
    zero_plan.stages.push_back(AffinePlanStage{
      0,
      0,
      0,
      0.0F,
      0.0F
    });
  }

  return JointMapPlanSegment{
    std::move(output_indices),
    std::move(zero_plan)
  };
}

} // namespace

tl::expected<JointMap, std::string> TransmissionAnalysisJointMapBuilder::build_expected(
  const std::vector<std::string> & input_names,
  const std::vector<std::string> & output_names,
  const JointQuantity quantity) const
{
  try {
    const auto & joint_order = transmission_analysis_.joint_order();
    const auto input_joint_ids = to_joint_ids_expected(joint_order, input_names, "input");
    const auto resolved_output_joint_ids = resolve_output_joint_ids(joint_order, output_names);

    JointMapPlan joint_map_plan{};
    if (!resolved_output_joint_ids.known_joint_ids.empty()) {
      const auto known_joint_map_plan = make_joint_map_plan_expected(
        transmission_analysis_,
        span<const JointId>(input_joint_ids),
        span<const JointId>(resolved_output_joint_ids.known_joint_ids),
        quantity);
      if (!known_joint_map_plan.has_value()) {
        return tl::make_unexpected(known_joint_map_plan.error().message);
      }

      joint_map_plan = *known_joint_map_plan;
    } else {
      joint_map_plan.input_joint_ids = input_joint_ids;
      joint_map_plan.stages.push_back(JointMapPlanStage{
        input_joint_ids,
        {},
        {}
      });
    }

    if (!resolved_output_joint_ids.unknown_output_indices.empty()) {
      auto & final_stage = joint_map_plan.stages.back();
      final_stage.segments.push_back(
        make_zero_affine_segment(final_stage.input_joint_ids, resolved_output_joint_ids.unknown_output_indices));
    }

    return compile_joint_map_plan_expected(transmission_analysis_, joint_map_plan, quantity);
  } catch (const std::exception & error) {
    return tl::make_unexpected(error.what());
  }
}

} // namespace arm_kinematics
