// Created by Bailey Chessum on 19/04/2026.

#ifndef ARM_KINEMATICS_ROS2_CONTROL_INTERFACE_REFS_HPP
#define ARM_KINEMATICS_ROS2_CONTROL_INTERFACE_REFS_HPP

#include <functional>
#include <string>
#include <vector>

#include <hardware_interface/loaned_command_interface.hpp>
#include <hardware_interface/loaned_state_interface.hpp>
#include <tl_expected/expected.hpp>

#include "arm_kinematics/joint_map/state_interface_definition.hpp"
#include "arm_kinematics/ros2_control/interface_lookup_error.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics::ros2_control {

using LoanedStateRef =
  std::reference_wrapper<const hardware_interface::LoanedStateInterface>;
using LoanedCommandRef =
  std::reference_wrapper<hardware_interface::LoanedCommandInterface>;

/**
 * Acquire ordered state interface refs matching a list of `NamedStateInterfaceDefinition`s.
 *
 * For each definition, finds the interface where:
 *   `iface.get_prefix_name() == def.joint_name &&
 *    iface.get_interface_name() == def.interface_id.name`
 *
 * Returns refs in the same order as `definitions`. All missing interfaces are collected
 * before returning so the caller receives a complete list in a single error.
 *
 * \param state_interfaces  The controller's `state_interfaces_` vector from `on_activate`.
 * \param definitions       Ordered interface declarations (typically `state_inputs_`).
 * \returns Ordered refs on success; `InterfaceLookupError::MissingInterfaces` on failure.
 */
ARM_KINEMATICS_PUBLIC
tl::expected<std::vector<LoanedStateRef>, InterfaceLookupError>
find_state_interface_refs(
  std::vector<hardware_interface::LoanedStateInterface> & state_interfaces,
  span<const NamedStateInterfaceDefinition> definitions);

/**
 * Acquire ordered command interface refs matching a list of full interface name strings.
 *
 * For each name, finds the interface where `iface.get_name() == name`.
 *
 * Returns refs in the same order as `names`. All missing interfaces are collected before
 * returning.
 *
 * \param command_interfaces  The controller's `command_interfaces_` vector from `on_activate`.
 * \param names               Full interface names (e.g. "j1/position" or "ctrl/j1/position").
 * \returns Ordered refs on success; `InterfaceLookupError::MissingInterfaces` on failure.
 */
ARM_KINEMATICS_PUBLIC
tl::expected<std::vector<LoanedCommandRef>, InterfaceLookupError>
find_command_interface_refs(
  std::vector<hardware_interface::LoanedCommandInterface> & command_interfaces,
  span<const std::string> names);

}  // namespace arm_kinematics::ros2_control

#endif  // ARM_KINEMATICS_ROS2_CONTROL_INTERFACE_REFS_HPP
