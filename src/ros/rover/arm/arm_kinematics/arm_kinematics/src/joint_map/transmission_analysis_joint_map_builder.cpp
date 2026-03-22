//
// Created by Bailey Chessum on 22/03/2026.
//

#include "arm_kinematics/joint_map/transmission_analysis_joint_map_builder.hpp"

#include "arm_kinematics/joint_map/composite_joint_map.hpp"

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
    const auto & joint_order = transmission_analysis_.joint_order();
    const auto input_joint_ids = to_joint_ids_expected(joint_order, input_names, "input");
    const auto output_joint_ids = to_joint_ids_expected(joint_order, output_names, "output");

    const auto joint_map_plan = make_joint_map_plan_expected(
      transmission_analysis_,
      span<const JointId>(input_joint_ids),
      span<const JointId>(output_joint_ids),
      quantity);
    if (!joint_map_plan.has_value()) {
      return tl::make_unexpected(joint_map_plan.error());
    }

    return compile_joint_map_plan_expected(transmission_analysis_, *joint_map_plan, quantity);
  } catch (const std::exception & error) {
    return tl::make_unexpected(error.what());
  }
}

} // namespace arm_kinematics
