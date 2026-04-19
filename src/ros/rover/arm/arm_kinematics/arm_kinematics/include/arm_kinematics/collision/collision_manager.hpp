//
// Created by nova on 11/30/25.
//

#ifndef ARM_KINEMATICS_COLLISION_MANAGER_HPP
#define ARM_KINEMATICS_COLLISION_MANAGER_HPP

#include <limits>

#include "arm_kinematics/visibility_control.h"
#include "arm_kinematics/collision/collision_build_error.hpp"
#include "arm_kinematics/collision/collision_config.hpp"
#include "discrete_collision_plugin.hpp"

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
  [[nodiscard]] bool collide(
    std::vector<std::pair<size_t, size_t>> & colliding_pairs,
    size_t max_colliding_pairs = std::numeric_limits<size_t>::max()) const;
  void update_poses(const std::vector<double> & joint_states);

private:
  ForwardKinematicsPlugin::Tree::SharedPtr tree_{};
  DiscreteCollisionPlugin::SharedPtr plugin_{};
  Isometry3dVector collider_poses_{};
};

/**
 * Method to create a collision manager.
 * Not a constructor, as this might fail!
 * \param loader The plugin loader to use to load the collision plugin instance
 * \param fk The FK plugin to make a tree to determine the poses for all colliders
 * \param joint_names The names of joints actuated in the fk tree provided as argument to .update_poses()
 * \returns The CollisionManager if everything could be created. A structured build error otherwise.
 */
tl::expected<CollisionManager, MakeCollisionError> make_collision_manager(
  PluginLoader & loader,
  const ForwardKinematicsPlugin::SharedPtr & fk,
  const std::vector<std::string> & joint_names);

tl::expected<CollisionManager, MakeCollisionError> make_collision_manager(
  PluginLoader & loader,
  const ForwardKinematicsPlugin::SharedPtr & fk,
  const std::vector<std::string> & joint_names,
  const CollisionConfig & config);

/**
 * Method to create a collision manager.
 * Not a constructor, as this might fail!
 * \param loader The plugin loader to use to load the collision plugin instance
 * \param fk The FK plugin to make a tree to determine the poses for all colliders
 * \note joint_names are retrieved from loader.get_kinematics_params()->joint_names in this overload.
 * \returns The CollisionManager if everything could be created. A structured build error otherwise.
 */
tl::expected<CollisionManager, MakeCollisionError> make_collision_manager(
  PluginLoader & loader,
  const ForwardKinematicsPlugin::SharedPtr & fk);

tl::expected<CollisionManager, MakeCollisionError> make_collision_manager(
  PluginLoader & loader,
  const ForwardKinematicsPlugin::SharedPtr & fk,
  const CollisionConfig & config);

} // arm_kinematics

#endif //ARM_KINEMATICS_COLLISION_MANAGER_HPP
