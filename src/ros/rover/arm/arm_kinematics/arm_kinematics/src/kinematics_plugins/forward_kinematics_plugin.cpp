//
// Created by Bailey Chessum on 14/10/2025.
//

#include <arm_kinematics/kinematics_plugins/forward_kinematics_plugin.hpp>

#include <urdf/model.h>
#include <stdexcept>

#include <arm_kinematics/utilities.hpp>
#include "arm_kinematics/joint_map/joint_map_builder.hpp"
#include <kdl/chainfksolverpos_recursive.hpp>


namespace arm_kinematics {

bool ForwardKinematicsPlugin::initialize(
  KinematicsNodeInterfaces node_interfaces,
  std::string & robot_description,
  KinematicsParams kinematics_params)
{
  if (!initialize_base(node_interfaces, robot_description, std::move(kinematics_params), "forward_kinematics"))
    return false;

  // Set up URDF
  RCLCPP_INFO(get_logger(), "Parsing URDF and creating KDL Tree...");
  if (!urdf_model_.initString(robot_description)) {
    RCLCPP_ERROR(get_logger(), "Failed to init URDF model from robot_description string.");
    return false;
  }

  joint_map_builder_ = JointMapBuilder()
    .with_urdf(urdf_model_)
    .with_transmissions(robot_description, get_logger());

  return on_initialize();
}

const urdf::Model & ForwardKinematicsPlugin::get_urdf_model() const noexcept {
  return urdf_model_;
}

const JointMapBuilder & ForwardKinematicsPlugin::get_joint_map_builder() const noexcept {
  return joint_map_builder_;
}

} // arm_kinematics