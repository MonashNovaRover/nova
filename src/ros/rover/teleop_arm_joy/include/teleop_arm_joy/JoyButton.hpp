//
// Created by Bailey Chessum on 25/1/25.
//

#ifndef JOYBUTTON_HPP
#define JOYBUTTON_HPP

#include <string>
#include <vector>
#include <sensor_msgs/msg/joy.hpp>

#include "JoyMessageListener.hpp"
#include "teleop_arm_joy_parameters.hpp"


namespace teleop_arm_joy
{

/**
* @brief Class that represents some button from the joy_node
*/
class JoyButton : public JoyMessageListener
{
public:
  /**
   * @brief Constructor for the joy Button.
   */
  explicit JoyButton(const Params::Buttons::MapButtonDefinitions &config);

  void joyCallback(sensor_msgs::msg::Joy::SharedPtr joy_msg) override;
  void debounce(const rclcpp::Time& now) override;

  [[nodiscard]] bool value();

  /**
   * @brief Gets if this button was just pressed down.
   */
  [[nodiscard]] bool down() const;

private:
  long id_;

  std::optional<rclcpp::Time> last_press_time_;

  bool current_value_ = false;
  bool debounce_previous_value_ = false;
  // True when for this most recent call of debounce() that the button is considered to have just been pressed
  bool debounce_down_ = false;
};

}

#endif //JOYBUTTON_HPP