//
// Created by Codex on 19/04/2026.
//

#ifndef ARM_KINEMATICS_COLLISION_UTILITIES_HPP
#define ARM_KINEMATICS_COLLISION_UTILITIES_HPP

#include <string_view>

#include <rclcpp/node_interfaces/node_parameters_interface.hpp>

#include "arm_kinematics/collision/allowed_collision_matrix.hpp"
#include "arm_kinematics/collision/collision_config.hpp"
#include "arm_kinematics/collision/discrete_collision_plugin.hpp"
#include "arm_kinematics/forward/forward_kinematics_plugin.hpp"
#include "arm_kinematics/utilities/span.hpp"

namespace arm_kinematics {

ARM_KINEMATICS_PUBLIC void probe_and_allow_collisions(
  DiscreteCollisionPlugin & plugin,
  ForwardKinematicsPlugin::Tree & tree,
  span<const double> joint_values);

ARM_KINEMATICS_PUBLIC void allow_collision_pairs_by_link(
  AllowedCollisionMatrix & acm,
  span<const std::string> parent_link_names,
  span<const std::pair<std::string, std::string>> allowed_pairs);

ARM_KINEMATICS_PUBLIC CollisionConfig read_collision_config(
  rclcpp::node_interfaces::NodeParametersInterface::SharedPtr params,
  std::string_view prefix = "collision");

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_COLLISION_UTILITIES_HPP
