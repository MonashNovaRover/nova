//
// Created by Bailey Chessum on 22/03/2026.
//

#include "arm_kinematics/joint_map/ros2_control_transmission_plugin_loader.hpp"

#include <stdexcept>

namespace arm_kinematics {

pluginlib::ClassLoader<transmission_interface::TransmissionLoader> &
Ros2ControlTransmissionPluginLoader::get_loader() const
{
  if (!loader_) {
    loader_ = std::make_unique<pluginlib::ClassLoader<transmission_interface::TransmissionLoader>>(
      "transmission_interface",
      "transmission_interface::TransmissionLoader");
  }

  return *loader_;
}

std::vector<std::string> Ros2ControlTransmissionPluginLoader::get_declared_plugin_types() const
{
  std::scoped_lock lock(mutex_);
  return get_loader().getDeclaredClasses();
}

bool Ros2ControlTransmissionPluginLoader::has_plugin_type(const std::string & plugin_type) const noexcept
{
  try {
    std::scoped_lock lock(mutex_);
    return get_loader().isClassAvailable(plugin_type);
  } catch (const std::exception &) {
    return false;
  }
}

std::shared_ptr<transmission_interface::TransmissionLoader> Ros2ControlTransmissionPluginLoader::make_loader(
  const std::string & plugin_type) const
{
  std::scoped_lock lock(mutex_);
  return get_loader().createSharedInstance(plugin_type);
}

std::shared_ptr<transmission_interface::Transmission> Ros2ControlTransmissionPluginLoader::load(
  const hardware_interface::TransmissionInfo & transmission_info) const
{
  auto loader = make_loader(transmission_info.type);
  auto transmission = loader->load(transmission_info);
  if (!transmission) {
    throw std::runtime_error(
      "ros2_control TransmissionLoader '" + transmission_info.type + "' returned a null Transmission");
  }

  return transmission;
}

} // namespace arm_kinematics
