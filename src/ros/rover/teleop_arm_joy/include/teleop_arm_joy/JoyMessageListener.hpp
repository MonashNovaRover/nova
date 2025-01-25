//
// Created by Bailey Chessum on 25/1/25.
//

#ifndef JOYMESSAGELISTENER_HPP
#define JOYMESSAGELISTENER_HPP

#include <sensor_msgs/msg/joy.hpp>

namespace teleop_arm_joy {

/**
 * @brief Abstract base class for any input class types.
 */
class JoyMessageListener {
public:
  virtual ~JoyMessageListener() = default;

  // TODO: Add distinct step for debouncing / updating, so that we can sync between devices

  virtual void joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg) = 0;
};

} // teleop_arm_joy

#endif //JOYMESSAGELISTENER_HPP
