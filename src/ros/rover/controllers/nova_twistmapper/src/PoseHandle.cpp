//
// Created by Bailey Chessum on 4/5/25.
// Helper struct to just collate the different component command interfaces
//

#include <optional>
#include "nova_twistmapper/PoseHandle.hpp"

void nova_twistmapper::PoseHandle::VectorHandle::set_value(const tf2::Vector3 &vec) const {
  x.command.get().set_value(vec.x());
  y.command.get().set_value(vec.y());
  z.command.get().set_value(vec.z());
}

void nova_twistmapper::PoseHandle::QuatHandle::set_value(const tf2::Quaternion &quat) const {
  x.command.get().set_value(quat.x());
  y.command.get().set_value(quat.y());
  z.command.get().set_value(quat.z());
  w.command.get().set_value(quat.w());
}

void nova_twistmapper::PoseHandle::set_value(const tf2::Transform &transform) const {
  origin.set_value(transform.getOrigin());
  rotation.set_value(transform.getRotation());
}

std::reference_wrapper<hardware_interface::LoanedCommandInterface> nova_twistmapper::PoseHandle::find_command_interface(
  std::vector<hardware_interface::LoanedCommandInterface>& command_interfaces,
  const std::string& name) {

  const auto command_handle = std::find_if(
    command_interfaces.begin(), command_interfaces.end(),
    [&name](const auto &interface)
    {
      return interface.get_name() == name;
    });

  if (command_handle == command_interfaces.end()) {
    throw std::runtime_error("Command interface not found: " + name);
  }

  return std::ref(*command_handle);
}

nova_twistmapper::PoseHandle nova_twistmapper::PoseHandle::pose_handle_from_command_interfaces(
  std::vector<hardware_interface::LoanedCommandInterface> &command_interfaces, const std::string &prefix) {
  return PoseHandle(
    find_command_interface(command_interfaces, prefix + "x"),
    find_command_interface(command_interfaces, prefix + "y"),
    find_command_interface(command_interfaces, prefix + "z"),
    find_command_interface(command_interfaces, prefix + "qx"),
    find_command_interface(command_interfaces, prefix + "qy"),
    find_command_interface(command_interfaces, prefix + "qz"),
    find_command_interface(command_interfaces, prefix + "qw"));
}
