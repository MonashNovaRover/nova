//
// Created by Bailey Chessum on 25/1/25.
//

#ifndef JOYDEVICE_HPP
#define JOYDEVICE_HPP

#include <string>
#include <vector>
#include <rclcpp/subscription.hpp>
#include <sensor_msgs/msg/joy.hpp>
using namespace std;

#include "JoyMessageListener.hpp"

namespace teleop_arm_joy {

/**
 * @brief A class representing a device definition in the config.yaml, and owns everything for that specific device.
 */
class JoyDevice {
public:

  /**
   * @brief constructor for the JoyDevice class, which manages the subscription to a joy node.
   * @param name The name of this device in the config
   * @param listeners The set of buttons and axes that will need to be updated whenever a joy message is received from
   * this device.
   */
  JoyDevice(const string &name, const vector<shared_ptr<JoyMessageListener>> &listeners);

private:
  string name;
  vector<shared_ptr<JoyMessageListener>> listeners;
  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub;

  // TODO: Add distinct step for debouncing / updating, so that we can sync between devices

  /**
   * @brief Callback function for joystick messages.
   * @param joy_msg Shared pointer to the joystick message.
   */
  void joyCallback(sensor_msgs::msg::Joy::SharedPtr joy_msg);
};

} // teleop_arm_joy

#endif //JOYDEVICE_HPP
