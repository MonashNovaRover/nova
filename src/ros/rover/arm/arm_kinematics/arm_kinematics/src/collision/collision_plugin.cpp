//
// Created by Bailey Chessum on 15/10/2025.
//

#include <arm_kinematics/collision/collision_plugin.hpp>

namespace arm_kinematics {

bool CollisionPlugin::initialize(KinematicsBase::KinematicsNodeInterfaces node_interfaces,
                                 const ForwardKinematicsPlugin::SharedPtr & fk) {
  if (!fk)
    return false;

  fk_ = fk;
  node_interfaces_ = node_interfaces;
  logger_ = node_interfaces.get_node_logging_interface()->get_logger().get_child("collision");

  return on_initialize();
}

const ForwardKinematicsPlugin::SharedPtr & CollisionPlugin::get_fk() const noexcept {
  return fk_;
}

const rclcpp::Logger & CollisionPlugin::get_logger() const noexcept {
  return logger_;
}

const CollisionPlugin::CollisionNodeInterfaces & CollisionPlugin::get_node_interfaces() const {
  if (!node_interfaces_.has_value())
    throw std::logic_error("Used a collision plugin before calling initialize() or after initialize() failed.");

  return *node_interfaces_;
}

} // arm_kinematics