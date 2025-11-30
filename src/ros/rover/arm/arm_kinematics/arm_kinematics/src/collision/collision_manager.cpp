//
// Created by nova on 11/30/25.
//

#include <arm_kinematics/collision/collision_manager.hpp>
#include <arm_kinematics/utilities/to_eigen.hpp>

namespace arm_kinematics {

bool CollisionManager::collide(const std::vector<double>& joint_states) {
  tree_->position_fk(joint_states, collider_poses_);
  return plugin_->collide({collider_poses_.data(), collider_poses_.size()});
}

} // arm_kinematics