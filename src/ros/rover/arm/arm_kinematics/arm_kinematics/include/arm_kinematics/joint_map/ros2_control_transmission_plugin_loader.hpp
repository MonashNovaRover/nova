//
// Created by Bailey Chessum on 22/03/2026.
//

#ifndef ARM_KINEMATICS_ROS2_CONTROL_TRANSMISSION_PLUGIN_LOADER_HPP
#define ARM_KINEMATICS_ROS2_CONTROL_TRANSMISSION_PLUGIN_LOADER_HPP

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <hardware_interface/hardware_info.hpp>
#include <pluginlib/class_loader.hpp>
#include <transmission_interface/transmission.hpp>
#include <transmission_interface/transmission_loader.hpp>

#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * \brief Reusable adapter around the ros2_control transmission plugin system.
 *
 * This class owns the pluginlib ClassLoader for transmission_interface::TransmissionLoader plugins, and provides
 * helpers to inspect the available plugin types and to load a transmission instance from a
 * hardware_interface::TransmissionInfo.
 *
 * \note This is a setup-time utility. Runtime mapping should use the resulting wrapped transmission instances rather
 * than calling back into pluginlib.
 */
class ARM_KINEMATICS_PUBLIC Ros2ControlTransmissionPluginLoader {
public:
  Ros2ControlTransmissionPluginLoader() = default;

  /// Returns the currently declared ros2_control transmission plugin types.
  [[nodiscard]] std::vector<std::string> get_declared_plugin_types() const;

  /// Returns true if a ros2_control transmission plugin type is available to be loaded.
  [[nodiscard]] bool has_plugin_type(const std::string & plugin_type) const noexcept;

  /// Creates a ros2_control TransmissionLoader plugin instance for the given plugin type.
  [[nodiscard]] std::shared_ptr<transmission_interface::TransmissionLoader> make_loader(
    const std::string & plugin_type) const;

  /// Uses the ros2_control plugin system to create a transmission instance for the given TransmissionInfo.
  [[nodiscard]] std::shared_ptr<transmission_interface::Transmission> load(
    const hardware_interface::TransmissionInfo & transmission_info) const;

private:
  [[nodiscard]] pluginlib::ClassLoader<transmission_interface::TransmissionLoader> & get_loader() const;

  mutable std::mutex mutex_{};
  mutable std::unique_ptr<pluginlib::ClassLoader<transmission_interface::TransmissionLoader>> loader_{};
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_ROS2_CONTROL_TRANSMISSION_PLUGIN_LOADER_HPP
