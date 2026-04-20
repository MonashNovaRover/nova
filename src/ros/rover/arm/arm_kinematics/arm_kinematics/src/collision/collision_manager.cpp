//
// Created by nova on 11/30/25.
//

#include "arm_kinematics/collision/collision_manager.hpp"
#include "arm_kinematics/collision/collision_utilities.hpp"
#include "arm_kinematics/plugin_loader.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
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

bool CollisionManager::collide(
  std::vector<std::pair<size_t, size_t>> & colliding_pairs,
  const size_t max_colliding_pairs) const
{
  return plugin_->collide(colliding_pairs, max_colliding_pairs);
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
  return make_collision_manager(loader, fk, joint_names, CollisionConfig{});
}

tl::expected<CollisionManager, MakeCollisionError> make_collision_manager(
  PluginLoader & loader,
  const ForwardKinematicsPlugin::SharedPtr & fk,
  const std::vector<std::string> & joint_names,
  const CollisionConfig & config)
{
  auto result = loader.make_collision(joint_names, fk);
  if (!result) {
    return tl::unexpected(std::move(result.error()));
  }
  auto [tree, plugin, parent_link_names] = std::move(result.value());

  if (config.generate_from_default_pose) {
    std::vector<double> joint_values(joint_names.size(), 0.0);
    for (const auto & [joint_name, value] : config.default_pose_overrides) {
      const auto it = std::find(joint_names.begin(), joint_names.end(), joint_name);
      if (it == joint_names.end()) {
        RCLCPP_WARN(
          rclcpp::get_logger("arm_kinematics.collision"),
          "Skipping collision default-pose override for unknown joint '%s'.",
          joint_name.c_str());
        continue;
      }
      joint_values[static_cast<std::size_t>(std::distance(joint_names.begin(), it))] = value;
    }
    probe_and_allow_collisions(*plugin, *tree, {joint_values.data(), joint_values.size()});
  }

  if (!config.allowed_pairs.empty()) {
    allow_collision_pairs_by_link(
      plugin->get_allowed_collision_matrix(),
      {parent_link_names.data(), parent_link_names.size()},
      {config.allowed_pairs.data(), config.allowed_pairs.size()});
  }

  return CollisionManager{
    std::move(tree),
    std::move(plugin)
  };
}

tl::expected<CollisionManager, MakeCollisionError> make_collision_manager(
  PluginLoader & loader,
  const ForwardKinematicsPlugin::SharedPtr & fk)
{
  return make_collision_manager(loader, fk, loader.get_kinematics_params()->joint_names, CollisionConfig{});
}

tl::expected<CollisionManager, MakeCollisionError> make_collision_manager(
  PluginLoader & loader,
  const ForwardKinematicsPlugin::SharedPtr & fk,
  const CollisionConfig & config)
{
  return make_collision_manager(loader, fk, loader.get_kinematics_params()->joint_names, config);
}

bool check_path_collision(
  CollisionManager & manager,
  const span<const double> start,
  const span<const double> end,
  const double step_size)
{
  if (start.size() != end.size()) {
    throw std::invalid_argument("check_path_collision() received start/end vectors with different sizes");
  }
  if (step_size <= 0.0) {
    throw std::invalid_argument("check_path_collision() requires step_size > 0");
  }

  double max_displacement = 0.0;
  for (std::size_t i = 0; i < start.size(); ++i) {
    max_displacement = std::max(max_displacement, std::abs(end[i] - start[i]));
  }

  const int iterations = static_cast<int>(std::ceil(max_displacement / step_size));
  std::vector<double> intermediate_positions(start.size(), 0.0);

  for (int i = 1; i < iterations; ++i) {
    const double interpolator = static_cast<double>(i) / static_cast<double>(iterations);
    const double one_minus_interpolator = 1.0 - interpolator;

    for (std::size_t j = 0; j < intermediate_positions.size(); ++j) {
      intermediate_positions[j] = start[j] * one_minus_interpolator + end[j] * interpolator;
    }

    manager.update_poses(intermediate_positions);
    if (manager.collide()) {
      return true;
    }
  }

  manager.update_poses(std::vector<double>(end.begin(), end.end()));
  return manager.collide();
}

} // arm_kinematics
