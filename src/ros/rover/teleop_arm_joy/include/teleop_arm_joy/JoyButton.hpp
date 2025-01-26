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

  [[nodiscard]] bool value() const;

  /**
   * @brief Gets if this button was just pressed down.
   */
  [[nodiscard]] bool down() const;

  /**
   * @brief Gets if this button was just released.
   */
  [[nodiscard]] bool up() const;

private:
  long id;

  bool currentValue = false;
  bool previousValue = false;
};

}

#endif //JOYBUTTON_HPP