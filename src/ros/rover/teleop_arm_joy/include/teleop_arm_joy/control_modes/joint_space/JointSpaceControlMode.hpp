//
// Created by Bailey Chessum on 6/4/25.
//

#ifndef JOINTSPACECONTROLMODE_HPP
#define JOINTSPACECONTROLMODE_HPP

#include "joint_space_control_mode_parameters.hpp"
#include "teleop_arm_joy/control_modes/ControlMode.hpp"
#include "nova_interfaces/msg/arm_fk_velocity_targets.hpp"
// generate_parameter_library_cpp include/teleop_arm_joy/control_modes/joint_space/joint_space_control_mode_parameters.hpp src/control_modes/joint_space/joint_space_control_mode_parameters.yaml

namespace teleop_arm_joy {
/**
 * Control mode for moving joint velocities directly
 */
class JointSpaceControlMode final : public ControlMode {

public:
  explicit JointSpaceControlMode() = default;

  void on_initialize() override;

  void on_configure(InputManager& inputs) override;
  void on_activate() override;
  void on_deactivate() override;

  void publish_halt_message(const rclcpp::Time& now) const;

  void update(const rclcpp::Time& now, const rclcpp::Duration& period) override;

protected:
  struct JointHandle {
    std::string name;
    joint_space_control_mode::Params::Joints::MapJointDefinitions config;
    Input<double>::SharedPtr input;
  };

  ~JointSpaceControlMode() = default;

  /// Tracks parameters
  std::shared_ptr<joint_space_control_mode::ParamListener> param_listener_;
  joint_space_control_mode::Params params_;

  // Used to just debug output various inputs
  std::vector<std::weak_ptr<Input<double>>> axes_{};
  std::vector<std::weak_ptr<Input<bool>>> buttons_{};

  /// Input from 0 to 1 that directly scales the output speed.
  Input<double>::SharedPtr speed_coefficient_;
  Input<bool>::SharedPtr locked_;

  /// Inputs for each joint in params_.joint_definitions, in the same order as params_.joint_definitions
  std::vector<JointHandle> joints_;

  /// Publishes messages to the arm controller
  rclcpp::Publisher<nova_interfaces::msg::ArmFkVelocityTargets>::SharedPtr publisher_;
};

} // teleop_arm_joy

#endif //JOINTSPACECONTROLMODE_HPP
