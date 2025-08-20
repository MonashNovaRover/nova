//
// Created by Bailey Chessum on 20/8/25.
//

#include "blcmd_hardware2/zero_manager.hpp"
#include <rclcpp/logging.hpp>
#include <cstdlib>   // for getenv
#include <filesystem>

namespace blcmd_hardware2 {

// Stinky stinky global variable to help share resources if possible
static ZeroManager shared{};

ZeroManager & ZeroManager::get_instance(const std::string& robot_name) {
  std::lock_guard<std::mutex> lock(shared.mtx);

  if (!shared.initialized_) {
    shared.init(robot_name);
  }

  return shared;
}

double ZeroManager::get_zero(const std::string &name) {
  std::lock_guard<std::mutex> lock(shared.mtx);

  const auto it = zeroes_.find(name);

  if (it == zeroes_.end())
    return 0.0;

  return it->second;
}

void ZeroManager::init(const std::string &robot_name) {
  const auto logger = rclcpp::get_logger("blcmd_hardware2/zero_manager");
  RCLCPP_INFO(logger, "Initializing Zero Manager");

  robot_name_ = robot_name;

  // TODO: Try load zeroes file

  // Build path: $HOME/.local/share/<robot_name>/zero_values.yaml
  const char* home = std::getenv("HOME");
  if (!home) {
    RCLCPP_ERROR(logger, "HOME environment variable not set");
    return;
  }

  std::filesystem::path file_path =
    std::filesystem::path(home) / ".local" / "share" / robot_name / "zeroes.yaml";

  RCLCPP_INFO(logger, "Looking for zero file at: %s", file_path.string().c_str());

  if (std::filesystem::exists(file_path)) {
    try {
      zeroes_yaml_ = YAML::LoadFile(file_path.string());

      for (auto it = zeroes_yaml_.begin(); it != zeroes_yaml_.end(); ++it) {
        const auto joint_name = it->first.as<std::string>();
        const auto joint_zero = it->second.as<double>();

        RCLCPP_INFO(logger, "  - %s: %f", joint_name.c_str(), joint_zero);
        zeroes_[joint_name] = joint_zero;
      }
    }
    catch (const std::exception &e) {
      RCLCPP_ERROR(logger, "Failed to read zero file: %s", e.what());
    }
  } else {
    RCLCPP_WARN(logger, "Zero values file does not exist.");
  }

  initialized_ = true;
}

}

