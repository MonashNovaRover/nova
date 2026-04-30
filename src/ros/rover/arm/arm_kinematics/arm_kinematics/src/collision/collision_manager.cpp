//
// Created by nova on 11/30/25.
//

#include "arm_kinematics/collision/collision_manager.hpp"
#include "arm_kinematics/collision/collision_utilities.hpp"
#include "arm_kinematics/plugin_loader.hpp"

#include <algorithm>
#include <cmath>
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

std::string PathCollisionError::SizeMismatch::format() const
{
  return
    "check_path_collision() received start/end vectors with different sizes (" +
    std::to_string(start_size) + " vs " + std::to_string(end_size) + ")";
}

std::string PathCollisionError::InvalidStepSize::format() const
{
  return "check_path_collision() requires step_size > 0, got " + std::to_string(step_size);
}

std::string PathCollisionError::ScratchSizeMismatch::format() const
{
  return
    "check_path_collision() received scratch vector with wrong size (" +
    std::to_string(scratch_size) + " vs expected " + std::to_string(state_size) + ")";
}

std::string PathCollisionError::format() const
{
  return std::visit([](const auto & error) { return error.format(); }, value);
}

CollisionManager::CollisionManager(
  ForwardKinematicsPlugin::Tree::SharedPtr tree,
  DiscreteCollisionPlugin::SharedPtr plugin,
  std::vector<std::string> parent_link_names)
: tree_(std::move(tree)),
  plugin_(std::move(plugin)),
  collider_poses_(plugin_->size()),
  parent_link_names_(std::move(parent_link_names))
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

void CollisionManager::update_poses(const span<const double> joint_states) {
  tree_->position_fk(joint_states, collider_poses_);
  plugin_->update_poses(0, {collider_poses_.data(), collider_poses_.size()});
}

const std::vector<std::string> & CollisionManager::parent_link_names() const noexcept
{
  return parent_link_names_;
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
  auto result = loader.make_collision(
    joint_names, fk,
    span<const std::string>(config.ignored_links.data(), config.ignored_links.size()));
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
    std::move(plugin),
    std::move(parent_link_names)
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

tl::expected<bool, PathCollisionError> check_path_collision(
  CollisionManager & manager,
  const span<const double> start,
  const span<const double> end,
  const double step_size,
  span<double> scratch)
{
  if (start.size() != end.size()) {
    return tl::unexpected(PathCollisionError::SizeMismatch{start.size(), end.size()});
  }
  if (step_size <= 0.0) {
    return tl::unexpected(PathCollisionError::InvalidStepSize{step_size});
  }
  if (scratch.size() != start.size()) {
    return tl::unexpected(PathCollisionError::ScratchSizeMismatch{start.size(), scratch.size()});
  }

  double max_displacement = 0.0;
  for (std::size_t i = 0; i < start.size(); ++i) {
    max_displacement = std::max(max_displacement, std::abs(end[i] - start[i]));
  }

  const int iterations = static_cast<int>(std::ceil(max_displacement / step_size));

  for (int i = 1; i < iterations; ++i) {
    const double interpolator = static_cast<double>(i) / static_cast<double>(iterations);
    const double one_minus_interpolator = 1.0 - interpolator;

    for (std::size_t j = 0; j < scratch.size(); ++j) {
      scratch[j] = start[j] * one_minus_interpolator + end[j] * interpolator;
    }

    manager.update_poses(scratch);
    if (manager.collide()) {
      return true;
    }
  }

  manager.update_poses(end);
  return manager.collide();
}

tl::expected<bool, PathCollisionError> check_path_collision(
  CollisionManager & manager,
  const span<const double> start,
  const span<const double> end,
  const double step_size,
  span<double> scratch,
  std::vector<std::pair<size_t, size_t>> & colliding_pairs)
{
  if (start.size() != end.size()) {
    return tl::unexpected(PathCollisionError::SizeMismatch{start.size(), end.size()});
  }
  if (step_size <= 0.0) {
    return tl::unexpected(PathCollisionError::InvalidStepSize{step_size});
  }
  if (scratch.size() != start.size()) {
    return tl::unexpected(PathCollisionError::ScratchSizeMismatch{start.size(), scratch.size()});
  }
  colliding_pairs.clear();

  double max_displacement = 0.0;
  for (std::size_t i = 0; i < start.size(); ++i) {
    max_displacement = std::max(max_displacement, std::abs(end[i] - start[i]));
  }

  const int iterations = static_cast<int>(std::ceil(max_displacement / step_size));

  for (int i = 1; i < iterations; ++i) {
    const double interpolator = static_cast<double>(i) / static_cast<double>(iterations);
    const double one_minus_interpolator = 1.0 - interpolator;

    for (std::size_t j = 0; j < scratch.size(); ++j) {
      scratch[j] = start[j] * one_minus_interpolator + end[j] * interpolator;
    }

    manager.update_poses(scratch);
    colliding_pairs.clear();
    if (manager.collide(colliding_pairs)) {
      return true;
    }
  }

  manager.update_poses(end);
  colliding_pairs.clear();
  return manager.collide(colliding_pairs);
}

tl::expected<bool, PathCollisionError> check_path_collision(
  CollisionManager & manager,
  const span<const double> start,
  const span<const double> end,
  const double step_size)
{
  std::vector<double> scratch(start.size());
  return check_path_collision(manager, start, end, step_size, scratch);
}

tl::expected<bool, PathCollisionError> check_path_collision(
  CollisionManager & manager,
  const span<const double> start,
  const span<const double> end,
  const double step_size,
  std::vector<std::pair<size_t, size_t>> & colliding_pairs)
{
  std::vector<double> scratch(start.size());
  return check_path_collision(manager, start, end, step_size, scratch, colliding_pairs);
}

} // arm_kinematics
