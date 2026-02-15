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
 * Helper class to tie together an FK tree to get collider poses, and a collision plugin.
 *
 * Create an instance of this with make_collision_manager() -- (see bottom of file)
 *
 * If you want to change allowed collisions, mess with plugin.get_allowed_collision_matrix()
 */
struct ARM_KINEMATICS_PUBLIC CollisionManager {
  CollisionManager() = default;

  CollisionManager(
    ForwardKinematicsPlugin::Tree::SharedPtr tree,
    DiscreteCollisionPlugin::SharedPtr plugin,
    size_t joint_count);

  /// Returns true if there is an intersection, and false otherwise
  [[nodiscard]] bool collide() const;
  void update_poses(const std::vector<double> & joint_states);

  /**
   * Perform a self intersection check with the given joint states, preserving which colliders would intersect.
   * Included for helping to build the allowed collision matrix. Unlimited colliding pairs.
   *
   * \warning This is much more expensive than the other function overload! Use only when you need the pairs.
   * \param[out] colliding_pairs Collision pairs found in collision
   * \param max_colliding_pairs The maximum number of pairs to populate colliding pairs with.
   * \returns true if there is an intersection, false if there is no intersection
   */
  bool get_colliding_pairs(
    std::vector<std::pair<size_t, size_t>> & colliding_pairs,
    const size_t max_colliding_pairs = std::numeric_limits<size_t>::max()) const
  {
    return plugin->collide(colliding_pairs, max_colliding_pairs);
  }

  /**
   * Gets the current colliding pairs for the current collider poses, and allows those colliding pairs in the allowed
   * collision matrix.
   *
   * \warning Not real-time safe.
   */
  void allow_current_colliding_pairs() const {
    std::vector<std::pair<size_t, size_t>> pairs;
    plugin->collide(pairs);
    for (const auto [i, j] : pairs) {
      plugin->get_allowed_collision_matrix().set(i, j, true);
    }
  }

  ForwardKinematicsPlugin::Tree::SharedPtr tree{};
  DiscreteCollisionPlugin::SharedPtr plugin{};
  Isometry3fVector collider_poses{};
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