
#include "teleop_arm_joy/control_modes/ControlMode.hpp"

void teleop_arm_joy::ControlMode::configure() {
  // Do common configuration

  // Perform child class configuration
  on_configure();
}
