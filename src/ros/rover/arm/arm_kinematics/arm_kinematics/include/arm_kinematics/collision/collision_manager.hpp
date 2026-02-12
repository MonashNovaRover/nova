//
// Created by nova on 11/30/25.
//

#ifndef ARM_KINEMATICS_COLLISION_MANAGER_HPP
#define ARM_KINEMATICS_COLLISION_MANAGER_HPP

#include "arm_kinematics/visibility_control.h"
#include "discrete_collision_plugin.hpp"
#include "arm_kinematics/forward/forward_kinematics_plugin.hpp"

namespace arm_kinematics {

class PluginLoader;

/**
 * Helper class to tie together an FK tree to get collider poses, and a collision plugin
 */
struct ARM_KINEMATICS_PUBLIC CollisionManager {
  CollisionManager() = default;

  CollisionManager(
    ForwardKinematicsPlugin::Tree::SharedPtr tree,
    DiscreteCollisionPlugin::SharedPtr plugin);

  [[nodiscard]] bool collide() const;
  void update_poses(const std::vector<double> & joint_states);

private:
  ForwardKinematicsPlugin::Tree::SharedPtr tree_{};
  DiscreteCollisionPlugin::SharedPtr plugin_{};
  Isometry3fVector collider_poses_{};
};

/**
 * Method to create a collision manager.
 * Not a constructor, as this might fail!
 * \param loader The plugin loader to use to load the collision plugin instance
 * \param fk The FK plugin to make a tree to determine the poses for all colliders
 * \param joint_names The names of joints actuated in the fk tree provided as argument to .update_poses()
 * \returns The CollisionManager if everything could be created. An error message otherwise.
 */
static tl::expected<CollisionManager, const char *> make_collision_manager(
  PluginLoader & loader,
  const ForwardKinematicsPlugin::SharedPtr & fk,
  const std::vector<std::string> & joint_names);

/**
 * Method to create a collision manager.
 * Not a constructor, as this might fail!
 * \param loader The plugin loader to use to load the collision plugin instance
 * \param fk The FK plugin to make a tree to determine the poses for all colliders
 * \note joint_names are retrieved from loader.get_kinematics_params()->joint_names in this overload.
 * \returns The CollisionManager if everything could be created. An error message otherwise.
 */
static tl::expected<CollisionManager, const char *> make_collision_manager(
  PluginLoader & loader,
  const ForwardKinematicsPlugin::SharedPtr & fk);

} // arm_kinematics

#endif //ARM_KINEMATICS_COLLISION_MANAGER_HPP