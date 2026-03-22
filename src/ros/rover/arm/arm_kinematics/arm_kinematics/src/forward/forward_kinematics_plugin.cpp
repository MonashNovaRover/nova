//
// Created by Bailey Chessum on 14/10/2025.
//

#include "arm_kinematics/forward/forward_kinematics_plugin.hpp"
#include <urdf/model.h>
#include <stdexcept>

namespace arm_kinematics {

bool ForwardKinematicsPlugin::initialize(
  const KinematicsNodeInterfaces & node_interfaces,
  const RobotModel & robot_model,
  KinematicsParams::SharedPtr kinematics_params)
{
  if (!initialize_base(node_interfaces, robot_model, std::move(kinematics_params), "forward_kinematics"))
    return false;

  // Set up URDF
  RCLCPP_INFO(get_logger(), "Parsing URDF and creating KDL Tree...");

  return on_initialize();
}

const TransmissionAnalysis & ForwardKinematicsPlugin::get_transmission_analysis() const noexcept
{
  return get_robot_model().get_default_transmission_analysis();
}

const JointMapBuilder & ForwardKinematicsPlugin::get_joint_map_builder() const noexcept
{
  if (!joint_map_builder_) {
    joint_map_builder_ = std::make_unique<TransmissionAnalysisJointMapBuilder>(get_transmission_analysis());
  }

  return *joint_map_builder_;
}

} // arm_kinematics
