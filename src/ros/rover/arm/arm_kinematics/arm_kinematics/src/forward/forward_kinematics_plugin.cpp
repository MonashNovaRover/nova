//
// Created by Bailey Chessum on 14/10/2025.
//

#include "arm_kinematics/forward/forward_kinematics_plugin.hpp"
#include <urdf/model.h>
#include <stdexcept>
#include "arm_kinematics/joint_map/joint_map_builder.hpp"

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

const JointMapBuilder & ForwardKinematicsPlugin::get_joint_map_builder() const noexcept {
  return get_robot_model().get_joint_map_builder();
}

} // arm_kinematics