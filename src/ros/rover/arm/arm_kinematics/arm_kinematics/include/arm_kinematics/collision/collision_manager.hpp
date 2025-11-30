//
// Created by nova on 11/30/25.
//

#ifndef ARM_KINEMATICS_COLLISION_MANAGER_HPP
#define ARM_KINEMATICS_COLLISION_MANAGER_HPP

#include <arm_kinematics/plugin_loader.hpp>

namespace arm_kinematics {

/**
 * Helper class to tie together an FK tree to get collider poses, and a collision plugin
 */
class CollisionManager {
public:
  CollisionManager(
    ForwardKinematicsPlugin::Tree::SharedPtr tree,
    CollisionPlugin::SharedPtr plugin)
      : tree_(std::move(tree)), plugin_(std::move(plugin)), collider_poses_(plugin_->size())
  {
  }

  bool collide(const std::vector<double> & joint_states);

private:
  ForwardKinematicsPlugin::Tree::SharedPtr tree_{};
  CollisionPlugin::SharedPtr plugin_{};
  Isometry3dVector collider_poses_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_COLLISION_MANAGER_HPP