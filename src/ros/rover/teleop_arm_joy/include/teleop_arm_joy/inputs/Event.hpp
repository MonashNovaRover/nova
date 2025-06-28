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
  using SharedPtr = std::shared_ptr<Event>;

  virtual ~Event() = default;

  virtual void invoke() {
    invoked_ = true;
  };

  /**
   * Called to mark the end of the frame, and update the value of is_invoked()
   */
  void update() {
    previous_invoked_ = invoked_;
    invoked_ = false;
  }

  virtual bool is_invoked() {
    return previous_invoked_;
  }

  // Type conversion
  explicit operator bool() {
    return is_invoked();
  }

private:
  bool previous_invoked_ = false;
  bool invoked_ = false;
};

}

#endif // EVENT_HPP
