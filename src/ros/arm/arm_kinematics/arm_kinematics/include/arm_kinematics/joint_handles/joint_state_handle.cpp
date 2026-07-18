//
// Created by nova on 12/28/25.
//

#include "joint_state_handle.hpp"

#include <hardware_interface/types/hardware_interface_type_values.hpp>

namespace arm_kinematics {

JointStateHandle::JointStateHandle(
  const std::string& joint_name,
  const std::vector<hardware_interface::LoanedStateInterface>& state_interfaces)
{
  const auto predicate = [&joint_name](const auto &interface)
  {
    return interface.get_prefix_name() == joint_name;
  };

  auto current = state_interfaces.cbegin();
  short unsigned int remaining = 3;   //< 1 for position, velocity, effort

  while (current != state_interfaces.cend() && remaining)
  {
    current = std::find_if(current, state_interfaces.cend(), predicate);

    if (current == state_interfaces.cend())
      break;

    if (current->get_interface_name() == hardware_interface::HW_IF_ACCELERATION)
    {
      acceleration = &*current;
      --remaining;
    }
    else if (current->get_interface_name() == hardware_interface::HW_IF_EFFORT)
    {
      effort = &*current;
      --remaining;
    }
    else if (current->get_interface_name() == hardware_interface::HW_IF_POSITION)
    {
      position = &*current;
      --remaining;
    }
    else if (current->get_interface_name() == hardware_interface::HW_IF_VELOCITY)
    {
      velocity = &*current;
      --remaining;
    }
  }

}

} // arm_kinematics