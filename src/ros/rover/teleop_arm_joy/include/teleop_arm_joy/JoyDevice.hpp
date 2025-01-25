//
// Created by nova on 1/25/25.
//

#ifndef JOYDEVICE_HPP
#define JOYDEVICE_HPP

#include <string>
#include <sensor_msgs/msg/joy.hpp>

#include "teleop_arm_joy_parameters.hpp"

namespace teleop_arm_joy {

/**
 * @brief A class representing a device definition in the config.yaml, and owns everything for that specific device.
 */
class JoyDevice {
  public:
    std::string name;

    /**
     * @brief constructor for the JoyDevice class, which calls this.initialize()
     * @param name The name of this device in the config
     * @param config The data defined under the device name in the config.yaml for this device
     */
    JoyDevice(std::string name, Params::DeviceMappings::MapDevices config) {
      this.name = name;
      this.initialize(config);
    }

    void initialize(Params::DeviceMappings::MapDevices config);

  private:
    /**
     * @brief Callback function for joystick messages.
     * @param joy_msg Shared pointer to the joystick message.
     */
    void joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg);
};

} // teleop_arm_joy

#endif //JOYDEVICE_HPP
