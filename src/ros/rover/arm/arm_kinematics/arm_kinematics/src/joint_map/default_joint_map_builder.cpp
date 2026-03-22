//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/default_joint_map_builder.hpp"

#include "arm_kinematics/joint_map/transmission_analysis_joint_map_builder.hpp"

namespace arm_kinematics {

tl::expected<JointMap, std::string> DefaultJointMapBuilder::build_expected(
  const std::vector<std::string> & input_names,
  const std::vector<std::string> & output_names,
  const JointQuantity quantity) const
{
  return TransmissionAnalysisJointMapBuilder(transmission_analysis_).build_expected(
    input_names,
    output_names,
    quantity);
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
