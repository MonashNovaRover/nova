/**
 * @file teleop_arm_joy.hpp
 * @brief Header file for the TeleopArmJoy class, which handles joystick input for teleoperation of the robotic arm.
 * Last Edited by Bailey
 */
#ifndef TELEOP_ARM_JOY_HPP
#define TELEOP_ARM_JOY_HPP

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <nova_interfaces/msg/arm_fk_velocity_targets.hpp>
#include <controller_manager_msgs/srv/switch_controller.hpp>

#include "teleop_arm_joy_parameters.hpp"
#include "JoyDevice.hpp"
#include "JoyButton.hpp"
#include "JoyAxis.hpp"

namespace teleop_arm_joy
{
  /**
   * @enum ControlMode
   * @brief Enum class for different control modes.
   */
  enum class ControlMode
  {
    FK,
    IK
  };

  inline std::string prettyPrintMode(const ControlMode mode)
  {
    switch (mode)
    {
    case ControlMode::FK:
      return "FK";
    case ControlMode::IK:
      return "IK";
    default:
      return "Unknown Mode";
    }
  }

  inline std::string modeToController(const ControlMode mode)
  {
    switch (mode)
    {
    case ControlMode::FK:
      return "";  // TODO
    case ControlMode::IK:
      return "";  // TODO
    default:
      return "unknown_controller";
    }
  }

  /**
   * @brief Struct representing the current state.
   */
  struct State
  {
    bool locked = true;

  };

  /**
   * @class TeleopArmJoy
   * @brief Class for handling joystick input and publishing arm commands.
   */
  class TeleopArmJoy : public rclcpp::Node
  {
  public:
    /**
     * @brief Constructor for TeleopArmJoy.
     * @param options Node options for the ROS2 node.
     */
    explicit TeleopArmJoy(const rclcpp::NodeOptions &options = rclcpp::NodeOptions());

    void initializeParams();

  private:
    void onDeviceUpdated(string& device_name);

    /**
     * @brief Update controller independent input state (control mode, locked, speed, etc)
     */
    void updateState();

    /**
     * @brief Sends Commands for the arm based on joystick input, and current control mode
     */
    void sendArmCommand();

    /**
     * @brief Sends a joint space velocity commands for the arm based on joystick input.
     */
    void sendJointSpaceCommand();

    /**
     * @brief Sends a task space twist command for the arm based on joystick input.
     */
    void sendTwistCommand();

    /**
     * @brief Sends a halt command to stop the rover.
     * Called when locked.
     */
    void sendHaltCommand();

    /**
     * @brief Handles changes in speed based on joystick input.
     */
    void handleSpeedChange();

    /**
     * @brief Switches the controller by calling the switch_controller service.
     * @param requested_control_mode The desired control mode to switch to.
     */
    void switchController(ControlMode requested_control_mode);

    // Member variables
    // rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub;
    rclcpp::Client<controller_manager_msgs::srv::SwitchController>::SharedPtr switch_controller_client;
    rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr fk_client;
    rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr ik_client;
    std::shared_ptr<ParamListener> param_listener_;

    rclcpp::Publisher<nova_interfaces::msg::ArmFkVelocityTargets>::SharedPtr fk_velocity_pub;
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr ik_twist_pub;

    std::vector<shared_ptr<JoyDevice>> devices;
    std::map<std::string, shared_ptr<JoyButton>> buttons;
    std::map<std::string, shared_ptr<JoyAxis>> axes;

    Params params_;
    State current_state;
    State previous_state;

    ControlMode control_mode = ControlMode::FK;
    double speed; // Linear Speed Multiplier that can be incremented
  };

} // namespace teleop_arm_joy

#endif // TELEOP_ARM_JOY_HPP