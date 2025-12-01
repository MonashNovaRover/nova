//
// Created by nova on 11/30/25.
//

#include <arm_kinematics/collision/collision_manager.hpp>
#include <arm_kinematics/plugin_loader.hpp>

namespace arm_kinematics {

CollisionManager::CollisionManager(
  ForwardKinematicsPlugin::Tree::SharedPtr tree,
  DiscreteCollisionPlugin::SharedPtr plugin)
: tree_(std::move(tree)),
  plugin_(std::move(plugin)),
  collider_poses_(plugin_->size())
{
}

CollisionManager::CollisionManager(
  PluginLoader & loader,
  const ForwardKinematicsPlugin::SharedPtr & fk,
  const std::vector<std::string> & joint_names)
{
  auto [tree, plugin] = loader.make_collision(joint_names, fk);
  tree_ = std::move(tree);
  plugin_ = std::move(plugin);
  collider_poses_.resize(plugin_->size());
}

bool CollisionManager::collide() {
  return plugin_->collide();
}

void CollisionManager::update_poses(const std::vector<double> & joint_states) {
  tree_->position_fk(joint_states, collider_poses_);
  plugin_->update_poses(0, {collider_poses_.data(), collider_poses_.size()});
}

} // arm_kinematics