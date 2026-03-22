//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/default_joint_map_builder.hpp"

#include "arm_kinematics/joint_map/affine_joint_map.hpp"

#include <stdexcept>

namespace arm_kinematics {

namespace {

struct BoundaryJointIds {
  std::vector<JointId> ids{};
  size_t known_count = 0;
};

std::string to_string(const JointQuantity quantity)
{
  switch (quantity) {
    case JointQuantity::Position:
      return "position";
    case JointQuantity::Velocity:
      return "velocity";
  }

  return "unknown";
}

BoundaryJointIds convert_joint_names_to_ids(
  const Order<std::string, JointId> & joint_ids,
  const std::vector<std::string> & joint_names)
{
  BoundaryJointIds result{};
  result.ids.reserve(joint_names.size());

  for (const auto & name : joint_names) {
    if (!joint_ids.contains_key(name))
      continue;

    result.ids.emplace_back(joint_ids[name]);
    ++result.known_count;
  }

  return result;
}

} // namespace

tl::expected<JointMap, std::string> DefaultJointMapBuilder::build_expected(
  const std::vector<std::string> & input_names,
  const std::vector<std::string> & output_names,
  const JointQuantity quantity) const
{
  try {
    return JointMap(AffineJointMap(input_names, output_names, mimic_joints_));
  } catch (const std::exception & e) {
    const auto & joint_ids = transmission_analysis_.joint_order();
    const auto input_ids = convert_joint_names_to_ids(joint_ids, input_names);
    const auto output_ids = convert_joint_names_to_ids(joint_ids, output_names);

    MakeTransmissionPlanResult maybe_plan = tl::make_unexpected(
      "Not all requested joint names are present in the transmission analysis boundary");
    if (input_ids.known_count == input_names.size() && output_ids.known_count == output_names.size()) {
      maybe_plan = make_transmission_plan_expected(
        transmission_analysis_,
        {input_ids.ids.data(), input_ids.ids.size()},
        {output_ids.ids.data(), output_ids.ids.size()},
        quantity);
    }

    if (maybe_plan.has_value()) {
      return tl::make_unexpected(
        "Transmission-backed JointMap build for " + to_string(quantity) +
        " was planned successfully, but runtime transmission mapping is not implemented yet");
    }
    const bool touches_transmission_analysis =
      input_ids.known_count > 0 || output_ids.known_count > 0;

    if (touches_transmission_analysis && !transmission_analysis_.transmissions().empty()) {
      return tl::make_unexpected(maybe_plan.error());
    }

    return tl::make_unexpected(std::string(e.what()));
  }
}

DefaultJointMapBuilder & DefaultJointMapBuilder::with_urdf(const urdf::Model & urdf_model)
{
  for (const auto & [name, joint] : urdf_model.joints_) {
    if (!joint->mimic)
      continue;

    mimic_joints_[name] = joint->mimic;
  }

  return *this;
}

DefaultJointMapBuilder & DefaultJointMapBuilder::with_transmissions(const std::string & urdf_string, rclcpp::Logger logger)
{
  transmissions_ = add_ros2_control_transmissions_to_analysis(transmission_analysis_, urdf_string, logger);
  return *this;
}

DefaultJointMapBuilder & DefaultJointMapBuilder::with_transmissions_dangerous(const std::string & urdf_string)
{
  transmissions_ = add_ros2_control_transmissions_to_analysis_dangerous(transmission_analysis_, urdf_string);
  return *this;
}

DefaultJointMapBuilder & DefaultJointMapBuilder::with_transmission_model(std::unique_ptr<TransmissionModel> model)
{
  transmission_analysis_.add_model(std::move(model));
  return *this;
}

const TransmissionAnalysis & DefaultJointMapBuilder::get_transmission_analysis() const noexcept
{
  return transmission_analysis_;
}

} // arm_kinematics
