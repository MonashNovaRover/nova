//
// Created by nova on 11/30/25.
//

#ifndef ARM_KINEMATICS_COLLISION_MANAGER_HPP
#define ARM_KINEMATICS_COLLISION_MANAGER_HPP
#include "collision_plugin.hpp"
#include "arm_kinematics/forward/forward_kinematics_plugin.hpp"

namespace arm_kinematics {

class PluginLoader;

/**
 * Helper class to tie together an FK tree to get collider poses, and a collision plugin
 */
class CollisionManager {
public:
  CollisionManager(
    ForwardKinematicsPlugin::Tree::SharedPtr tree,
    CollisionPlugin::SharedPtr plugin);

  CollisionManager(
    PluginLoader & loader,
    const ForwardKinematicsPlugin::SharedPtr & fk,
    const std::vector<std::string> & joint_names);

  bool collide(const std::vector<double> & joint_states);

private:
  ForwardKinematicsPlugin::Tree::SharedPtr tree_{};
  CollisionPlugin::SharedPtr plugin_{};
  Isometry3dVector collider_poses_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_COLLISION_MANAGER_HPP