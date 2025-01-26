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

  [[nodiscard]] float value() const;

private:
  long id;

  float currentValue = false;
};

}

#endif //JOYAXIS_HPP
