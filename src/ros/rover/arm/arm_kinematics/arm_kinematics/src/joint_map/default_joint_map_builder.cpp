//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/default_joint_map_builder.hpp"

#include "arm_kinematics/joint_map/affine_joint_map.hpp"

#include <stdexcept>

namespace arm_kinematics {

namespace {

} // namespace

tl::expected<JointMap, std::string> DefaultJointMapBuilder::build_expected(
  const std::vector<std::string> & input_names,
  const std::vector<std::string> & output_names,
  const JointQuantity quantity) const
{
  try {
    return JointMap(AffineJointMap(input_names, output_names, transmission_analysis_));
  } catch (const std::exception & e) {
    // TODO(nova): Re-enable builder-side grouped transmission orchestration once the underlying grouped transmission
    // planning and runtime structures are complete. Leaving the partial behavior enabled here would make this example
    // builder a semantic owner of incomplete logic, which is the opposite of the intended design.
    static_cast<void>(quantity);
    return tl::make_unexpected(std::string(e.what()));
  }
}

DefaultJointMapBuilder & DefaultJointMapBuilder::with_urdf(const urdf::Model & urdf_model)
{
  add_mimic_transmissions_to_analysis(transmission_analysis_, urdf_model);
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
