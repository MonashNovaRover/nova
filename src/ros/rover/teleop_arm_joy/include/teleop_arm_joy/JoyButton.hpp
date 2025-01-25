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
  explicit JoyButton(Params::DeviceMappings::MapDevices::Buttons::MapButtonDefinitions config);

  void joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg) override;

  bool value() const;

  /**
   * @brief Gets if this button was just pressed down.
   */
  bool down() const;

  /**
   * @brief Gets if this button was just released.
   */
  bool up() const;

private:
  int id;

  bool currentValue = false;
  bool previousValue = false;
};

}

#endif //JOYBUTTON_HPP