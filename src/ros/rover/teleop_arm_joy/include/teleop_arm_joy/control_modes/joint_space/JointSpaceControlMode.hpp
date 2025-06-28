//
// Created by Bailey Chessum on 6/4/25.
//

#ifndef JOINTSPACECONTROLMODE_HPP
#define JOINTSPACECONTROLMODE_HPP

#include "joint_space_control_mode_parameters.hpp"
#include "teleop_arm_joy/control_modes/ControlMode.hpp"
// generate_parameter_library_cpp include/teleop_arm_joy/control_modes/joint_space/joint_space_control_mode_parameters.hpp src/control_modes/joint_space/joint_space_control_mode_parameters.yaml

namespace teleop_arm_joy {
/**
 * Control mode for moving joint velocities directly
 */
class JointSpaceControlMode final : public ControlMode {

public:
  explicit JointSpaceControlMode() {}

  void on_initialize() override;

  void on_configure(InputManager& inputs) override;
  void on_activate() override;
  void on_deactivate() override;

  void update() override;

protected:
  ~JointSpaceControlMode() = default;

  std::shared_ptr<joint_space_control_mode::ParamListener> param_listener_;
  joint_space_control_mode::Params params_;

  std::vector<std::weak_ptr<Input<double>>> axes_{};
  std::vector<std::weak_ptr<Input<bool>>> buttons_{};



  // rclcpp::Publisher<nova_interfaces::msg::ArmFkVelocityTargets>::SharedPtr fk_velocity_pub_;
};

} // teleop_arm_joy

#endif //JOINTSPACECONTROLMODE_HPP
