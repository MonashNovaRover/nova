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
    /**
     * @brief Sends Commands for the arm based on joystick input.
     * @param joy_msg Shared pointer to the joystick message.
     */
    void sendArmCommand(const sensor_msgs::msg::Joy::SharedPtr joy_msg);

    /**
     * @brief Sends a halt command to stop the rover.
     * Called when locked.
     */
    void sendHaltCommand();

    /**
     * @brief Handles button callbacks.
     * @param joy_msg Shared pointer to the joystick message.
     */
    void handleButtonCallbacks(const sensor_msgs::msg::Joy::SharedPtr joy_msg);

    /**
     * @brief Handles changes in speed based on joystick input.
     *
     * @param joy_msg A shared pointer to the joystick message containing the input data.
     */
    void handleSpeedChange(const sensor_msgs::msg::Joy::SharedPtr joy_msg);

    /**
     * @brief Switches the controller by calling the switch_controller service.
     * @param requested_control_mode The desired control mode to switch to.
     */
    void switchController(const ControlMode requested_control_mode);

    // Member variables
    // rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub;
    rclcpp::Client<controller_manager_msgs::srv::SwitchController>::SharedPtr switch_controller_client;
    rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr fk_client;
    rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr ik_client;
    std::shared_ptr<ParamListener> param_listener_;

    std::vector<JoyDevice> devices;
    std::map<std::string, shared_ptr<JoyButton>> buttons;
    std::map<std::string, shared_ptr<JoyAxis>> axes;

    Params params_;
    bool sent_lock_msg = false;
    State current_state;
    State previous_state;
    ControlMode control_mode = ControlMode::FK;
    double speed; // Linear Speed Multiplier that can be incremented
    std::map<int, rclcpp::Time> last_button_press_time_;
  };

} // namespace teleop_arm_joy

#endif // TELEOP_ARM_JOY_HPP