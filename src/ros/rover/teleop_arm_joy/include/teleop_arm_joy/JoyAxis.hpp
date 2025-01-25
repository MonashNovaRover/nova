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
  JoyAxis(Params::DeviceMappings::MapDevices::Axes::MapAxisDefinitions config);

  void joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg) override;

  float value() const;

private:
  int id;

  float currentValue = false;
};

}

#endif //JOYAXIS_HPP
