// Created by Bailey Chessum on 19/04/2026.

#include "arm_kinematics/ros2_control/joint_interface_bundle.hpp"

#include <algorithm>
#include "hardware_interface/types/hardware_interface_type_values.hpp"

namespace arm_kinematics::ros2_control {

tl::expected<std::vector<JointInterfaceBundle>, InterfaceLookupError>
find_joint_interface_bundles(
  std::vector<hardware_interface::LoanedStateInterface> & state_interfaces,
  std::vector<hardware_interface::LoanedCommandInterface> & command_interfaces,
  span<const std::string> joint_names,
  bool require_position_state,
  bool require_velocity_state,
  std::string_view command_type)
{
  std::vector<JointInterfaceBundle> bundles;
  bundles.reserve(joint_names.size());

  std::vector<std::string> missing;

  for (const auto & joint_name : joint_names) {
    JointInterfaceBundle bundle;
    bundle.joint_name = joint_name;

    // --- Position state ---
    {
      const auto it = std::find_if(
        state_interfaces.begin(), state_interfaces.end(),
        [&joint_name](const hardware_interface::LoanedStateInterface & iface) {
          return iface.get_prefix_name() == joint_name &&
                 iface.get_interface_name() == hardware_interface::HW_IF_POSITION;
        });

      if (it == state_interfaces.end()) {
        if (require_position_state) {
          missing.push_back(joint_name + "/" + hardware_interface::HW_IF_POSITION);
        }
      } else {
        bundle.position = &(*it);
      }
    }

    // --- Velocity state ---
    {
      const auto it = std::find_if(
        state_interfaces.begin(), state_interfaces.end(),
        [&joint_name](const hardware_interface::LoanedStateInterface & iface) {
          return iface.get_prefix_name() == joint_name &&
                 iface.get_interface_name() == hardware_interface::HW_IF_VELOCITY;
        });

      if (it == state_interfaces.end()) {
        if (require_velocity_state) {
          missing.push_back(joint_name + "/" + hardware_interface::HW_IF_VELOCITY);
        }
      } else {
        bundle.velocity = &(*it);
      }
    }

    // --- Command (always required) ---
    {
      const auto it = std::find_if(
        command_interfaces.begin(), command_interfaces.end(),
        [&joint_name, &command_type](const hardware_interface::LoanedCommandInterface & iface) {
          return iface.get_prefix_name() == joint_name &&
                 iface.get_interface_name() == command_type;
        });

      if (it == command_interfaces.end()) {
        missing.push_back(std::string{joint_name} + "/" + std::string{command_type});
      } else {
        bundle.command = &(*it);
      }
    }

    bundles.push_back(std::move(bundle));
  }

  if (!missing.empty()) {
    return tl::unexpected(InterfaceLookupError::MissingInterfaces{std::move(missing)});
  }
  return bundles;
}

}  // namespace arm_kinematics::ros2_control
