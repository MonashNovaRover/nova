//
// Created by nova on 11/30/25.
//

#include "arm_kinematics/collision/collision_manager.hpp"
#include "arm_kinematics/plugin_loader.hpp"

#include <variant>

namespace arm_kinematics {

std::string MakeCollisionError::MakeTreeFailed::format() const
{
  return "CollisionManager build failed while creating the FK tree: " + error.format();
}

std::string MakeCollisionError::CollisionPluginInitFailed::format() const
{
  return detail;
}

std::string MakeCollisionError::format() const
{
  return std::visit([](const auto & error) { return error.format(); }, value);
}

CollisionManager::CollisionManager(
  ForwardKinematicsPlugin::Tree::SharedPtr tree,
  DiscreteCollisionPlugin::SharedPtr plugin)
: tree_(std::move(tree)),
  plugin_(std::move(plugin)),
  collider_poses_(plugin_->size())
{
}

bool CollisionManager::collide() const {
  return plugin_->collide();
}

void CollisionManager::update_poses(const std::vector<double> & joint_states) {
  tree_->position_fk(joint_states, collider_poses_);
  plugin_->update_poses(0, {collider_poses_.data(), collider_poses_.size()});
}

tl::expected<CollisionManager, MakeCollisionError> make_collision_manager(
  PluginLoader & loader,
  const ForwardKinematicsPlugin::SharedPtr & fk,
  const std::vector<std::string> & joint_names)
{
  auto result = loader.make_collision(joint_names, fk);
  if (!result) {
    return tl::unexpected(std::move(result.error()));
  }
  auto [tree, plugin] = std::move(result.value());

  return CollisionManager{
    std::move(tree),
    std::move(plugin)
  };
}

tl::expected<CollisionManager, MakeCollisionError> make_collision_manager(
  PluginLoader & loader,
  const ForwardKinematicsPlugin::SharedPtr & fk)
{
  return make_collision_manager(loader, fk, loader.get_kinematics_params()->joint_names);
}

} // arm_kinematics
