//
// Created by Bailey Chessum on 15/10/2025.
//

#include "arm_kinematics/collision/discrete_collision_plugin.hpp"

namespace arm_kinematics {

bool DiscreteCollisionPlugin::initialize(
  CollisionNodeInterfaces node_interfaces,
  const std::vector<std::reference_wrapper<const urdf::Collision>>& collider_geometries,
  AllowedCollisionMatrix acm)
{
  node_interfaces_ = node_interfaces;
  logger_ = node_interfaces.get_node_logging_interface()->get_logger().get_child("collision");
  acm_ = std::move(acm);
  size_ = collider_geometries.size();

  // Implementation specific initialization
  return on_initialize(collider_geometries);
}

const rclcpp::Logger & DiscreteCollisionPlugin::get_logger() const noexcept {
  return logger_;
}

const DiscreteCollisionPlugin::CollisionNodeInterfaces & DiscreteCollisionPlugin::get_node_interfaces() const {
  if (!node_interfaces_.has_value())
    throw std::logic_error("Used a collision plugin before calling initialize() or after initialize() failed.");

  return *node_interfaces_;
}

} // arm_kinematics