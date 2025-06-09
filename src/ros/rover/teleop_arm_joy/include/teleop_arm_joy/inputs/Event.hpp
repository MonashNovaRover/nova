//
// Created by Bailey Chessum on 4/6/25.
//

#ifndef EVENT_HPP
#define EVENT_HPP

#include <rclcpp/time.hpp>

namespace teleop_arm_joy
{

class Event {

public:
  virtual ~Event() = default;

  virtual void invoke() = 0;
private:
  bool invoked = false;
};

}

#endif // EVENT_HPP
