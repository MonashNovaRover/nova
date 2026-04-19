//
// Created by Codex on 19/04/2026.
//

#include "arm_kinematics/collision/collision_utilities.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <rclcpp/logging.hpp>

#include "arm_kinematics/utilities/param_reader.hpp"

namespace arm_kinematics {

namespace {

std::string trim_copy(std::string_view input)
{
  std::size_t start = 0;
  while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
    ++start;
  }

  std::size_t end = input.size();
  while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
    --end;
  }

  return std::string(input.substr(start, end - start));
}

std::string prefixed_name(std::string_view prefix, std::string_view suffix)
{
  if (prefix.empty()) {
    return std::string(suffix);
  }
  return std::string(prefix) + "." + std::string(suffix);
}

}  // namespace

void probe_and_allow_collisions(
  DiscreteCollisionPlugin & plugin,
  ForwardKinematicsPlugin::Tree & tree,
  span<const double> joint_values)
{
  std::vector<double> joint_state_vector(joint_values.begin(), joint_values.end());
  Isometry3dVector poses(plugin.size());
  tree.position_fk(joint_state_vector, poses);
  plugin.update_poses(0, {poses.data(), poses.size()});

  std::vector<std::pair<size_t, size_t>> colliding_pairs;
  if (!plugin.collide(colliding_pairs)) {
    return;
  }

  auto & acm = plugin.get_allowed_collision_matrix();
  for (const auto & [a, b] : colliding_pairs) {
    RCLCPP_DEBUG(
      plugin.get_logger(),
      "Allowing default-pose collision pair (%zu, %zu).",
      a,
      b);
    acm.set(a, b, true);
  }
}

void allow_collision_pairs_by_link(
  AllowedCollisionMatrix & acm,
  span<const std::string> parent_link_names,
  span<const std::pair<std::string, std::string>> allowed_pairs)
{
  constexpr auto kLoggerName = "arm_kinematics.collision";
  std::unordered_map<std::string, std::vector<size_t>> indices_by_link;
  indices_by_link.reserve(parent_link_names.size());
  for (size_t i = 0; i < parent_link_names.size(); ++i) {
    indices_by_link[parent_link_names[i]].push_back(i);
  }

  for (const auto & [link_a, link_b] : allowed_pairs) {
    const auto a_it = indices_by_link.find(link_a);
    const auto b_it = indices_by_link.find(link_b);
    if (a_it == indices_by_link.end() || b_it == indices_by_link.end()) {
      RCLCPP_WARN(
        rclcpp::get_logger(kLoggerName),
        "Skipping allowed collision pair (%s, %s): missing link name in collider definitions.",
        link_a.c_str(),
        link_b.c_str());
      continue;
    }

    for (const auto a_idx : a_it->second) {
      for (const auto b_idx : b_it->second) {
        acm.set(a_idx, b_idx, true);
      }
    }
  }
}

CollisionConfig read_collision_config(
  rclcpp::node_interfaces::NodeParametersInterface::SharedPtr params,
  std::string_view prefix)
{
  const ParamReader reader(std::move(params));
  CollisionConfig config;
  config.generate_from_default_pose = reader.get<bool>(
    prefixed_name(prefix, "generate_from_default_pose"),
    true);

  const auto default_pose_entries = reader.get<std::vector<std::string>>(
    prefixed_name(prefix, "default_pose_overrides"),
    {});
  for (const auto & entry : default_pose_entries) {
    const auto separator = entry.find('=');
    if (separator == std::string::npos) {
      RCLCPP_WARN(
        rclcpp::get_logger("arm_kinematics.collision"),
        "Skipping malformed collision default-pose override '%s'. Expected 'joint=value'.",
        entry.c_str());
      continue;
    }

    const auto joint_name = trim_copy(std::string_view(entry).substr(0, separator));
    const auto value_text = trim_copy(std::string_view(entry).substr(separator + 1));
    if (joint_name.empty() || value_text.empty()) {
      RCLCPP_WARN(
        rclcpp::get_logger("arm_kinematics.collision"),
        "Skipping malformed collision default-pose override '%s'.",
        entry.c_str());
      continue;
    }

    try {
      std::size_t parsed_chars = 0;
      const auto value = std::stod(value_text, &parsed_chars);
      if (parsed_chars != value_text.size()) {
        throw std::invalid_argument("trailing characters");
      }
      const auto existing = config.default_pose_overrides.find(joint_name);
      if (existing != config.default_pose_overrides.end()) {
        RCLCPP_WARN(
          rclcpp::get_logger("arm_kinematics.collision"),
          "Collision default-pose override for '%s' was specified more than once; using the last value.",
          joint_name.c_str());
      }
      config.default_pose_overrides[joint_name] = value;
    } catch (const std::exception &) {
      RCLCPP_WARN(
        rclcpp::get_logger("arm_kinematics.collision"),
        "Skipping collision default-pose override '%s': value is not a valid number.",
        entry.c_str());
    }
  }

  const auto allowed_pair_entries = reader.get<std::vector<std::string>>(
    prefixed_name(prefix, "allowed_pairs"),
    {});
  for (const auto & entry : allowed_pair_entries) {
    const auto separator = entry.find(',');
    if (separator == std::string::npos) {
      RCLCPP_WARN(
        rclcpp::get_logger("arm_kinematics.collision"),
        "Skipping malformed collision allowed-pair entry '%s'. Expected 'link_a,link_b'.",
        entry.c_str());
      continue;
    }

    auto link_a = trim_copy(std::string_view(entry).substr(0, separator));
    auto link_b = trim_copy(std::string_view(entry).substr(separator + 1));
    if (link_a.empty() || link_b.empty()) {
      RCLCPP_WARN(
        rclcpp::get_logger("arm_kinematics.collision"),
        "Skipping malformed collision allowed-pair entry '%s'.",
        entry.c_str());
      continue;
    }

    config.allowed_pairs.emplace_back(std::move(link_a), std::move(link_b));
  }

  return config;
}

}  // namespace arm_kinematics
