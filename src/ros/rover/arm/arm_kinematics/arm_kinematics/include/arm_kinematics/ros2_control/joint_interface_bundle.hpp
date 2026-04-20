// Created by Bailey Chessum on 19/04/2026.

#ifndef ARM_KINEMATICS_ROS2_CONTROL_JOINT_INTERFACE_BUNDLE_HPP
#define ARM_KINEMATICS_ROS2_CONTROL_JOINT_INTERFACE_BUNDLE_HPP

#include <string>
#include <string_view>
#include <vector>

#include <hardware_interface/loaned_command_interface.hpp>
#include <hardware_interface/loaned_state_interface.hpp>
#include <tl_expected/expected.hpp>

#include "arm_kinematics/ros2_control/interface_lookup_error.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics::ros2_control {

/**
 * Per-joint bundle of hardware interface pointers.
 *
 * State pointers are nullable — a nullptr indicates the interface was not required
 * (or not found when not required). `command` is always non-null on a successfully
 * acquired bundle.
 */
struct ARM_KINEMATICS_PUBLIC JointInterfaceBundle {
  std::string joint_name{};

  /// Position state interface. Non-null if and only if `require_position_state` was true
  /// and the interface was successfully found, OR if the interface was found when not required.
  const hardware_interface::LoanedStateInterface * position{nullptr};

  /// Velocity state interface. Non-null if and only if `require_velocity_state` was true
  /// and the interface was successfully found, OR if the interface was found when not required.
  const hardware_interface::LoanedStateInterface * velocity{nullptr};

  /// Command interface. Always non-null for a successfully returned bundle.
  hardware_interface::LoanedCommandInterface * command{nullptr};
};

/**
 * Acquire per-joint interface bundles from the controller's interface vectors.
 *
 * For each joint in `joint_names`:
 * - Searches for a position state interface where
 *   `get_prefix_name()==joint_name && get_interface_name()=="position"`.
 *   If `require_position_state` is true and not found → error.
 *   If found (regardless of requirement): fills `bundle.position`.
 * - Searches for a velocity state interface similarly with `"velocity"`.
 * - Searches for a command interface where
 *   `get_prefix_name()==joint_name && get_interface_name()==command_type`.
 *   The command interface is always required; if not found → error.
 *
 * All missing required interfaces are collected before returning so the caller gets a
 * complete list in a single error.
 *
 * \param state_interfaces       The controller's `state_interfaces_` vector.
 * \param command_interfaces     The controller's `command_interfaces_` vector.
 * \param joint_names            Ordered list of joint names to build bundles for.
 * \param require_position_state If true, missing position state interface is an error.
 * \param require_velocity_state If true, missing velocity state interface is an error.
 * \param command_type           Interface name for the command (e.g. `HW_IF_POSITION`).
 * \returns Ordered bundles on success; `InterfaceLookupError::MissingInterfaces` on failure.
 */
ARM_KINEMATICS_PUBLIC
tl::expected<std::vector<JointInterfaceBundle>, InterfaceLookupError>
find_joint_interface_bundles(
  std::vector<hardware_interface::LoanedStateInterface> & state_interfaces,
  std::vector<hardware_interface::LoanedCommandInterface> & command_interfaces,
  span<const std::string> joint_names,
  bool require_position_state,
  bool require_velocity_state,
  std::string_view command_type);

}  // namespace arm_kinematics::ros2_control

#endif  // ARM_KINEMATICS_ROS2_CONTROL_JOINT_INTERFACE_BUNDLE_HPP
