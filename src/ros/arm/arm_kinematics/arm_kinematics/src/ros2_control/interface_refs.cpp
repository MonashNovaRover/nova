// Created by Bailey Chessum on 19/04/2026.

#include "arm_kinematics/ros2_control/interface_refs.hpp"

#include <algorithm>

namespace arm_kinematics::ros2_control {

tl::expected<std::vector<LoanedStateRef>, InterfaceLookupError>
find_state_interface_refs(
  std::vector<hardware_interface::LoanedStateInterface> & state_interfaces,
  span<const NamedStateInterfaceDefinition> definitions)
{
  std::vector<LoanedStateRef> refs;
  refs.reserve(definitions.size());

  std::vector<std::string> missing;

  for (const auto & def : definitions) {
    const auto it = std::find_if(
      state_interfaces.begin(), state_interfaces.end(),
      [&def](const hardware_interface::LoanedStateInterface & iface) {
        return iface.get_prefix_name() == def.joint_name &&
               iface.get_interface_name() == def.interface_id.name;
      });

    if (it == state_interfaces.end()) {
      missing.push_back(def.joint_name + "/" + def.interface_id.name);
    } else {
      refs.emplace_back(std::ref(std::as_const(*it)));
    }
  }

  if (!missing.empty()) {
    return tl::unexpected(InterfaceLookupError::MissingInterfaces{std::move(missing)});
  }
  return refs;
}

tl::expected<std::vector<LoanedCommandRef>, InterfaceLookupError>
find_command_interface_refs(
  std::vector<hardware_interface::LoanedCommandInterface> & command_interfaces,
  span<const std::string> names)
{
  std::vector<LoanedCommandRef> refs;
  refs.reserve(names.size());

  std::vector<std::string> missing;

  for (const auto & name : names) {
    const auto it = std::find_if(
      command_interfaces.begin(), command_interfaces.end(),
      [&name](const hardware_interface::LoanedCommandInterface & iface) {
        return iface.get_name() == name;
      });

    if (it == command_interfaces.end()) {
      missing.push_back(name);
    } else {
      refs.emplace_back(std::ref(*it));
    }
  }

  if (!missing.empty()) {
    return tl::unexpected(InterfaceLookupError::MissingInterfaces{std::move(missing)});
  }
  return refs;
}

}  // namespace arm_kinematics::ros2_control
