// Created by Bailey Chessum on 19/04/2026.

#include "arm_kinematics/ros2_control/interface_lookup_error.hpp"

#include <sstream>
#include <variant>

#include <rcutils/logging_macros.h>

namespace arm_kinematics::ros2_control {

std::string InterfaceLookupError::MissingInterfaces::format() const
{
  std::ostringstream oss;
  oss << "Interface lookup failed: " << interface_names.size()
      << " interface(s) not found: [";
  for (std::size_t i = 0; i < interface_names.size(); ++i) {
    if (i > 0) {
      oss << ", ";
    }
    oss << interface_names[i];
  }
  oss << "]";
  return oss.str();
}

void InterfaceLookupError::MissingInterfaces::log(
  const rclcpp::Logger & logger,
  rclcpp::Logger::Level level) const
{
  const std::string msg = format();
  RCUTILS_LOG_COND_NAMED(
    static_cast<int>(level),
    RCUTILS_LOG_CONDITION_EMPTY,
    RCUTILS_LOG_CONDITION_EMPTY,
    logger.get_name(),
    "%s", msg.c_str());
}

std::string InterfaceLookupError::format() const
{
  return std::visit([](const auto & e) { return e.format(); }, value);
}

void InterfaceLookupError::log(
  const rclcpp::Logger & logger,
  rclcpp::Logger::Level level) const
{
  std::visit([&](const auto & e) { e.log(logger, level); }, value);
}

}  // namespace arm_kinematics::ros2_control
