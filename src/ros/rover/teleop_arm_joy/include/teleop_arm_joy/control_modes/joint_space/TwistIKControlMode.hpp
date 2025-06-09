//
// Created by Bailey Chessum on 6/4/25.
//

#ifndef TWISTIKCONTROLMODE_HPP
#define TWISTIKCONTROLMODE_HPP

#include "teleop_arm_joy/control_modes/ControlMode.hpp"

namespace teleop_arm_joy {
/**
 * Control mode for moving joint velocities directly
 */
class TwistIKControlMode final : public ControlMode {

public:
  explicit TwistIKControlMode() {}

  void on_initialize() override;

  void on_configure() override;
  void on_activate() override;
  void on_deactivate() override;

protected:
  ~TwistIKControlMode() = default;

  // rclcpp::Publisher<nova_interfaces::msg::ArmFkVelocityTargets>::SharedPtr fk_velocity_pub_;
};

} // teleop_arm_joy

#endif //TWISTIKCONTROLMODE_HPP
