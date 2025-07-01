//
// Created by Bailey Chessum on 10/6/25.
//

#include "teleop_arm_joy/input_sources/InputSource.hpp"

namespace teleop_arm_joy {

void InputSource::initialize(const std::shared_ptr<rclcpp::Node>& node, const std::string& name,
  const std::weak_ptr<InputSourceUpdateDelegate>& delegate)
{
  node_ = node;
  name_ = name;
  delegate_ = delegate;

  on_initialize();
}

void InputSource::update(const rclcpp::Time& now) {
  on_update(now);
}

void InputSource::request_update(const rclcpp::Time& now) const {
  const auto delegate = delegate_.lock();

  if (!delegate) {
    RCLCPP_FATAL(node_->get_logger(), "InputSource %s's delegate weak_ptr is invalid!", name_.c_str());
    return;
  }

  const auto time_to_send = now.nanoseconds() == 0 ? node_->get_clock()->now() : now;
  delegate->on_input_source_requested_update(time_to_send);
}
} // namespace teleop_arm_joy
