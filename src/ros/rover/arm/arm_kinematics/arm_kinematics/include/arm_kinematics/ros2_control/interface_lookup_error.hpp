// Created by Bailey Chessum on 19/04/2026.

#ifndef ARM_KINEMATICS_ROS2_CONTROL_INTERFACE_LOOKUP_ERROR_HPP
#define ARM_KINEMATICS_ROS2_CONTROL_INTERFACE_LOOKUP_ERROR_HPP

#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <rclcpp/logger.hpp>

#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics::ros2_control {

/**
 * Error returned when one or more hardware interfaces cannot be found during `on_activate`.
 *
 * Follows the project's variant-based error pattern: one concrete nested type per failure
 * mode, each with `format()` and `log(logger, level)`. The outer struct provides a
 * templated converting constructor so callers can return directly:
 *
 * \code
 *   return tl::unexpected(InterfaceLookupError::MissingInterfaces{{"shoulder/position"}});
 * \endcode
 */
struct ARM_KINEMATICS_PUBLIC InterfaceLookupError {
  /**
   * One or more declared interfaces were not present in the controller's interface vectors.
   * `interface_names` lists every unresolved name so the caller gets a complete picture
   * in a single error rather than discovering failures one-by-one.
   */
  struct MissingInterfaces {
    /// Full interface names (e.g. "shoulder/position") that could not be resolved.
    std::vector<std::string> interface_names{};

    [[nodiscard]] std::string format() const;

    /**
     * Log the error at the given severity level.
     * Defaults to ERROR, which is appropriate for setup-time lookup failures.
     */
    void log(
      const rclcpp::Logger & logger,
      rclcpp::Logger::Level level = rclcpp::Logger::Level::Error) const;
  };

  using Variant = std::variant<MissingInterfaces>;
  Variant value;

  template <
    typename T,
    typename Decayed = std::decay_t<T>,
    typename = std::enable_if_t<!std::is_same_v<Decayed, InterfaceLookupError>>>
  InterfaceLookupError(T && t)  // NOLINT(google-explicit-constructor)
  : value(std::forward<T>(t))
  {
  }

  [[nodiscard]] std::string format() const;

  void log(
    const rclcpp::Logger & logger,
    rclcpp::Logger::Level level = rclcpp::Logger::Level::Error) const;
};

}  // namespace arm_kinematics::ros2_control

#endif  // ARM_KINEMATICS_ROS2_CONTROL_INTERFACE_LOOKUP_ERROR_HPP
