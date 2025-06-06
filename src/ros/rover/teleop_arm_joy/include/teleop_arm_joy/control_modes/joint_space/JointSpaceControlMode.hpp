//
// Created by Bailey Chessum on 6/4/25.
//

#ifndef JOINTSPACECONTROLMODE_HPP
#define JOINTSPACECONTROLMODE_HPP

#include "teleop_arm_joy/control_modes/ControlMode.hpp"

namespace teleop_arm_joy {
/**
 * Control mode for moving joint velocities directly
 */
class JointSpaceControlMode final : public ControlMode {

public:
  explicit JointSpaceControlMode() {}

  void on_initialize() override;

  void on_configure() override;
  void on_activate() override;
  void on_deactivate() override;

protected:
  ~JointSpaceControlMode() = default;

  // rclcpp::Publisher<nova_interfaces::msg::ArmFkVelocityTargets>::SharedPtr fk_velocity_pub_;
};

} // teleop_arm_joy

#endif //JOINTSPACECONTROLMODE_HPP
