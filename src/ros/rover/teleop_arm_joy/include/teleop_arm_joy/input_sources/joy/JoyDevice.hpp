//
// Created by Bailey Chessum on 25/1/25.
//

#ifndef JOYDEVICE_HPP
#define JOYDEVICE_HPP

#include <string>
#include <vector>
#include <rclcpp/node.hpp>
#include <rclcpp/subscription.hpp>
#include <sensor_msgs/msg/joy.hpp>

#include "joy_input_source_parameters.hpp"
#include "../../JoyMessageListener.hpp"
using namespace std;


namespace teleop_arm_joy {

/**
 * @brief A class representing a device definition in the config.yaml, and owns everything for that specific device.
 */
class JoyDevice {
public:

  /**
   * @brief constructor for the JoyDevice class, which manages the subscription to a joy node.
   * @param parent The ROS2 node that owns this, so a subscription can be made
   * @param name The name of this device in the config
   * @param config The configuration of the given device from the config file
   * @param listeners The set of buttons and axes that will need to be updated whenever a joy message is received from
   * this device.
   * @param callback Callback function for after receiving a joy message and updating listeners
   */
  JoyDevice(rclcpp::Node* parent, const string& name, const joy_input_source::Params::Devices::MapDeviceNames& config, const vector<shared_ptr<JoyMessageListener>>& listeners, const function<void(string&)>& callback);

  // Accessors
  [[nodiscard]] const string& name() const {
    return name_;
  }

  void debounce();

private:
  string name_;
  vector<shared_ptr<JoyMessageListener>> listeners;
  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub;
  function<void(string&)> callback;
  rclcpp::Node* parent_;

  /**
   * @brief Callback function for joystick messages.
   * @param joy_msg Shared pointer to the joystick message.
   */
  void joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg);
};

} // teleop_arm_joy

#endif //JOYDEVICE_HPP
