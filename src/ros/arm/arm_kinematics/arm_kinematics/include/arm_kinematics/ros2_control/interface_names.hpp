// Created by Bailey Chessum on 19/04/2026.

#ifndef ARM_KINEMATICS_ROS2_CONTROL_INTERFACE_NAMES_HPP
#define ARM_KINEMATICS_ROS2_CONTROL_INTERFACE_NAMES_HPP

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include "arm_kinematics/joint_map/state_interface_definition.hpp"
#include "arm_kinematics/utilities/span.hpp"

namespace arm_kinematics::ros2_control {

/**
 * Build state interface name strings for `InterfaceConfiguration::names`.
 *
 * Returns one entry per (joint, type) combination in declaration order:
 *   state_interface_names({"j1","j2"}, {"position"})
 *   → ["j1/position", "j2/position"]
 *
 *   state_interface_names({"j1","j2"}, {"position","velocity"})
 *   → ["j1/position", "j1/velocity", "j2/position", "j2/velocity"]
 */
inline std::vector<std::string> state_interface_names(
  span<const std::string> joint_names,
  std::initializer_list<const char *> types)
{
  std::vector<std::string> names;
  names.reserve(joint_names.size() * types.size());
  for (const auto & joint : joint_names) {
    for (const char * type : types) {
      names.push_back(joint + "/" + type);
    }
  }
  return names;
}

/**
 * Build state interface name strings from `NamedStateInterfaceDefinition` declarations.
 *
 * Produces `def.joint_name + "/" + def.interface_id.name` for each definition.
 * Convenient when the same `state_inputs_` vector is used for both `make_tree()` and
 * `state_interface_configuration()`.
 */
inline std::vector<std::string> state_interface_names(
  span<const NamedStateInterfaceDefinition> defs)
{
  std::vector<std::string> names;
  names.reserve(defs.size());
  for (const auto & def : defs) {
    names.push_back(def.joint_name + "/" + def.interface_id.name);
  }
  return names;
}

/**
 * Build command interface name strings for `InterfaceConfiguration::names`.
 *
 * With no prefix:       "j1/position", "j2/position"
 * With prefix "ctrl":   "ctrl/j1/position", "ctrl/j2/position"
 *
 * The prefix corresponds to `params_.chained_controller_name` in chained controllers.
 */
inline std::vector<std::string> command_interface_names(
  span<const std::string> joint_names,
  std::string_view command_type,
  std::string_view chained_prefix = "")
{
  const bool has_prefix = !chained_prefix.empty();
  std::vector<std::string> names;
  names.reserve(joint_names.size());
  for (const auto & joint : joint_names) {
    std::string name;
    if (has_prefix) {
      name.reserve(chained_prefix.size() + 1 + joint.size() + 1 + command_type.size());
      name += chained_prefix;
      name += '/';
    } else {
      name.reserve(joint.size() + 1 + command_type.size());
    }
    name += joint;
    name += '/';
    name += command_type;
    names.push_back(std::move(name));
  }
  return names;
}

}  // namespace arm_kinematics::ros2_control

#endif  // ARM_KINEMATICS_ROS2_CONTROL_INTERFACE_NAMES_HPP
