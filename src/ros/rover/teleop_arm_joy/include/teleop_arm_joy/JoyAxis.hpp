//
// Created by Bailey Chessum on 25/1/25.
//

#ifndef JOYAXIS_HPP
#define JOYAXIS_HPP

#include <sensor_msgs/msg/joy.hpp>

#include "JoyMessageListener.hpp"
#include "teleop_arm_joy_parameters.hpp"
#include "Input.hpp"

namespace teleop_arm_joy
{

/**
* @brief Class that represents some axis from the joy_node
*/
class JoyAxis : public JoyMessageListener, Input<float> {
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
  bool invert = false;

  float current_value_ = 0.0f;
  float current_debounce_value_ = 0.0f;
  float last_debounce_value_ = 0.0f;

  long button_id_negative_ = -1;
  long button_id_positive_ = -1;
};

}

#endif //JOYAXIS_HPP
