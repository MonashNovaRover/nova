//
// Created by Bailey Chessum on 22/03/2026.
//

#include "arm_kinematics/joint_map/transmission_analysis_joint_map_builder.hpp"

#include "arm_kinematics/joint_map/affine_joint_map.hpp"
#include "arm_kinematics/joint_map/transmission_joint_map.hpp"

#include <stdexcept>

namespace arm_kinematics {

namespace {

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

} // namespace

tl::expected<JointMap, std::string> TransmissionAnalysisJointMapBuilder::build_expected(
  const std::vector<std::string> & input_names,
  const std::vector<std::string> & output_names,
  const JointQuantity quantity) const
{
  try {
    return JointMap(AffineJointMap(input_names, output_names, transmission_analysis_));
  } catch (const std::exception & e) {
    const auto affine_error = std::string(e.what());

    try {
      const auto & joint_order = transmission_analysis_.joint_order();
      const auto input_joint_ids = to_joint_ids_expected(joint_order, input_names, "input");
      const auto output_joint_ids = to_joint_ids_expected(joint_order, output_names, "output");

      const auto transmission_plan = make_transmission_plan_expected(
        transmission_analysis_,
        span<const JointId>(input_joint_ids),
        span<const JointId>(output_joint_ids),
        quantity);
      if (!transmission_plan.has_value()) {
        return tl::make_unexpected(affine_error + "; " + transmission_plan.error());
      }

      auto compiled_plan = compile_transmission_plan_expected(
        transmission_analysis_,
        *transmission_plan,
        quantity);
      if (!compiled_plan.has_value()) {
        return tl::make_unexpected(compiled_plan.error());
      }

      return JointMap(TransmissionJointMap(std::move(*compiled_plan)));
    } catch (const std::exception & grouped_error) {
      return tl::make_unexpected(affine_error + "; " + grouped_error.what());
    }
  }
}

} // namespace arm_kinematics
