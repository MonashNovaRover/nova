//
// Created by Bailey Chessum on 15/10/2025.
//

#include <arm_kinematics/kinematics_plugins/kinematics_base.hpp>

namespace arm_kinematics {

bool KinematicsBase::initialize_base(
  KinematicsNodeInterfaces node_interfaces,
  const std::string & robot_description,
  KinematicsParams params,
  const std::string& logger_name)
{
  robot_description_ = & robot_description;
  node_interfaces_ = node_interfaces;
  logger_ = node_interfaces.get_node_logging_interface()->get_logger().get_child(logger_name);

  // if (joint_names.empty()) {
  //   RCLCPP_ERROR(logger_, "Tried to initialize kinematics plugin with no joint names. You must define at least one.");
  //   return false;
  // }
  // joint_names_ = joint_names;

  kinematics_params_ = std::move(params);

  return true;
}

// const std::vector<std::string> &KinematicsBase::get_joint_names() const noexcept {
//   return joint_names_;
// }

const std::string & KinematicsBase::get_robot_description() const {
  if (!robot_description_)
    throw std::logic_error("Used a kinematics plugin before calling initialize() or after initialize() failed.");

  return *robot_description_;
}

const rclcpp::Logger &KinematicsBase::get_logger() const noexcept {
  return logger_;
}

const KinematicsParams &KinematicsBase::get_kinematics_params() const noexcept {
  return kinematics_params_;
}

const KinematicsBase::KinematicsNodeInterfaces & KinematicsBase::get_node_interfaces() const {
  if (!node_interfaces_.has_value())
    throw std::logic_error("Used a kinematics plugin before calling initialize() or after initialize() failed.");

  return *node_interfaces_;
}


} // arm_kinematics