//
// Created by Bailey Chessum on 6/4/25.
//

#ifndef JOINTSPACECONTROLMODE_HPP
#define JOINTSPACECONTROLMODE_HPP
#include <nova_interfaces/msg/detail/arm_fk_velocity_targets__struct.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

#include "teleop_arm_joy/control_modes/ControlMode.hpp"

namespace teleop_arm_joy {
/**
 * Control mode for moving joint velocities directly
 */
class JointSpaceControlMode : public ControlMode {

public:
  JointSpaceControlMode(const std::shared_ptr<rclcpp::node_interfaces::NodeBaseInterface>& node): ControlMode(node) {

  }

  virtual void on_configure() override;
  virtual void on_activate() override;
  virtual void on_deactivate() override;

protected:
  ~JointSpaceControlMode() = default;

  rclcpp::Publisher<nova_interfaces::msg::ArmFkVelocityTargets>::SharedPtr fk_velocity_pub_;


};

} // teleop_arm_joy

#endif //JOINTSPACECONTROLMODE_HPP
