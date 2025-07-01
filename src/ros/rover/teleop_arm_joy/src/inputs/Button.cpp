//
// Created by nova on 7/1/25.
//

#include "teleop_arm_joy/inputs/Button.hpp"

void teleop_arm_joy::Button::export_events(teleop_arm_joy::EventCollection &events) {
  on_pressed_down.reset();
  on_pressed.reset();
  on_pressed_up.reset();

  on_pressed_down = events[get_name() + "/down"];
  on_pressed = events[get_name()];
  on_pressed_up = events[get_name() + "/up"];
}

void teleop_arm_joy::Button::update_events(const rclcpp::Time &now) {
  if (!changed() || !on_pressed_down || !on_pressed_up) {
    return;
  }

  if (value()) {
    on_pressed_down->invoke();
    on_pressed->invoke();
  }
  else
    on_pressed_up->invoke();
}


