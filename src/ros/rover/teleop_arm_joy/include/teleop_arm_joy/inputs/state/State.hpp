//
// Created by nova on 6/15/25.
//

#ifndef STATE_HPP
#define STATE_HPP
#include "teleop_arm_joy/inputs/Input.hpp"

namespace teleop_arm_joy {

template<typename T>
class State : public Input<T> {
public:
  explicit State(const std::string& name, const T& initial_value) : Input<T>(name), value_(initial_value) {}

  T value() override {
    return value_;
  }

  void set(const T& new_value) {
    changed_since_debounce_ = changed_since_debounce_ ||  value_ != new_value;
    value_ = new_value;
  }

  void debounce(const rclcpp::Time& now) override {
    changed_last_debounce_ = changed_since_debounce_;
    changed_since_debounce_ = false;
  }

  bool changed() const override {
    return changed_last_debounce_;
  }

private:
  T value_;
  bool changed_since_debounce_ = false;
  bool changed_last_debounce_ = false;
};

} // teleop_arm_joy

#endif //STATE_HPP
