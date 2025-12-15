//
// Created by Bailey Chessum on 15/10/2025.
//

#include "arm_kinematics/common/kinematics_base.hpp"

namespace arm_kinematics {

bool KinematicsBase::initialize_base(
  KinematicsNodeInterfaces node_interfaces,
  const RobotModel & robot_model,
  KinematicsParams::SharedPtr params,
  const std::string & logger_name)
{
  node_interfaces_ = node_interfaces;
  logger_ = node_interfaces.get_node_logging_interface()->get_logger().get_child(logger_name);
  robot_model_ = &robot_model;

  kinematics_params_ = std::move(params);

  return true;
}

const RobotModel & KinematicsBase::get_robot_model() const {
  if (!robot_model_)
    throw std::logic_error("Used get_robot_model() on a kinematics plugin before calling initialize() or after "
                           "initialize() failed.");

  return *robot_model_;
}

const rclcpp::Logger & KinematicsBase::get_logger() const noexcept {
  return logger_;
}

const KinematicsParams & KinematicsBase::get_kinematics_params() const {
  if (!kinematics_params_)
    throw std::logic_error("Used get_kinematics_params() on a kinematics plugin before calling initialize() or after "
                           "initialize() failed.");

  return *kinematics_params_;
}

const KinematicsBase::KinematicsNodeInterfaces & KinematicsBase::get_node_interfaces() const {
  if (!node_interfaces_.has_value())
    throw std::logic_error("Used get_node_interfaces() on a kinematics plugin before calling initialize() or after "
                           "initialize() failed.");

  return *node_interfaces_;
}

} // arm_kinematics