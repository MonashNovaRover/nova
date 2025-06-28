/**
 * @file teleop_arm_joy.hpp
 * @brief Header file for the TeleopArmJoy class, which handles joystick input for teleoperation of the robotic arm.
 * Last Edited by Abby
 */
#ifndef TELEOP_ARM_JOY_HPP
#define TELEOP_ARM_JOY_HPP

#include <rclcpp/rclcpp.hpp>

// generate_parameter_library_cpp include/teleop_arm_joy/teleop_arm_joy_parameters.hpp src/parameters.yaml
#include "teleop_arm_joy_parameters.hpp"
#include "control_modes/ControlModeManager.hpp"
#include "inputs/state/State.hpp"
#include "input_sources/InputSourceManager.hpp"

namespace teleop_arm_joy
{
  /**
   * @class TeleopArmJoy
   * @brief Class for handling joystick and other inputs and publishing arm commands.
   */
  class TeleopArmJoy : public rclcpp::Node {
  public:
    /**
     * @brief Constructor for TeleopArmJoy.
     * @param options Node options for the ROS2 node.
     */
    explicit TeleopArmJoy(const rclcpp::NodeOptions &options = rclcpp::NodeOptions());
    ~TeleopArmJoy() override;

    void initialize(const std::weak_ptr<rclcpp::Executor>& executor);

    /**
     * Infinite loop that repeatedly services updates from input sources. The heart of the program.
     */
    [[noreturn]] void service_input_updates();

  private:
    void update_state();

    std::shared_ptr<ParamListener> param_listener_;
    Params params_;

    std::shared_ptr<ControlModeManager> control_mode_manager_ = nullptr;
    InputManager inputs_;
    std::shared_ptr<InputSourceManager> input_source_manager_ = nullptr;

    std::shared_ptr<State<bool>> locked_ = std::make_shared<State<bool>>(State("locked", true));
  };

} // namespace teleop_arm_joy

#endif // TELEOP_ARM_JOY_HPP
