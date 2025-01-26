//
// Created by Bailey Chessum on 25/1/25.
//

#ifndef JOYAXIS_HPP
#define JOYAXIS_HPP

#include <sensor_msgs/msg/joy.hpp>

#include "JoyMessageListener.hpp"
#include "teleop_arm_joy_parameters.hpp"

namespace teleop_arm_joy
{

/**
* @brief Class that represents some axis from the joy_node
*/
class JoyAxis : public JoyMessageListener {
public:

  /**
   * @brief Constructor for the joy axis.
   */
  explicit JoyAxis(const Params::Axes::MapAxisDefinitions &config);

  void joyCallback(sensor_msgs::msg::Joy::SharedPtr joy_msg) override;
  void debounce(const rclcpp::Time& now) override;

  [[nodiscard]] float value() const;

  /**
   * @returns true if the value changed since last debounce
   */
  [[nodiscard]] bool changed() const;

private:
  long id;

  float current_value_ = 0.0f;
  float current_debounce_value_ = 0.0f;
  float last_debounce_value_ = 0.0f;
};

}

#endif //JOYAXIS_HPP
