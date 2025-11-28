//
// Created by Bailey Chessum on 14/10/2025.
//

#include <arm_kinematics/forward/forward_kinematics_plugin.hpp>
#include <urdf/model.h>
#include <stdexcept>
#include <arm_kinematics/utilities/utilities.hpp>
#include <arm_kinematics/joint_map/joint_map_builder.hpp>

namespace arm_kinematics {

bool ForwardKinematicsPlugin::initialize(
  const KinematicsNodeInterfaces & node_interfaces,
  KinematicsParams::SharedPtr kinematics_params)
{
  if (!initialize_base(node_interfaces, std::move(kinematics_params), "forward_kinematics"))
    return false;

  // Set up URDF
  RCLCPP_INFO(get_logger(), "Parsing URDF and creating KDL Tree...");

  joint_map_builder_ = JointMapBuilder()
    .with_urdf(get_urdf_model())
    .with_transmissions(get_kinematics_params().robot_description, get_logger());

  return on_initialize();
}

const urdf::Model & ForwardKinematicsPlugin::get_urdf_model() const {
  return get_kinematics_params().get_urdf_model();
}

const JointMapBuilder & ForwardKinematicsPlugin::get_joint_map_builder() const noexcept {
  return joint_map_builder_;
}

} // arm_kinematics