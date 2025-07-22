/**
 * @file teleop_drive_joy.hpp
 * @brief Header file for the TeleopDriveJoy class, which handles joystick input for teleoperation.
 * Last Edited by Kabi
 */
#ifndef TELEOP_DRIVE_JOY_HPP
#define TELEOP_DRIVE_JOY_HPP

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <nova_interfaces/msg/drive_input_stamped.hpp>
#include <nova_interfaces/msg/drive_info.hpp>
#include <controller_manager_msgs/srv/switch_controller.hpp>

#include "teleop_drive_joy_parameters.hpp"

namespace teleop_drive_joy
{

  /**
   * @enum ControlMode
   * @brief Enum class for different control modes.
   */
  enum class ControlMode
  {
    PIVOT_DRIVE,  // Pivot Drive Controller
    STRAFE_DRIVE, // Strafe Controller
    DIFF_DRIVE    // Nova Diff Drive Controller
  };

  inline std::string prettyPrintMode(const ControlMode mode)
  {
    switch (mode)
    {
    case ControlMode::PIVOT_DRIVE:
      return "Pivot Mode";
    case ControlMode::STRAFE_DRIVE:
      return "Strafe Mode";
    case ControlMode::DIFF_DRIVE:
      return "Tank Mode";
    default:
      return "Unknown Mode";
    }
  }

  inline std::string modeToController(const ControlMode mode)
  {
    switch (mode)
    {
    case ControlMode::PIVOT_DRIVE:
      return "pivot_drive_controller";
    case ControlMode::STRAFE_DRIVE:
      return "strafe_controller";
    case ControlMode::DIFF_DRIVE:
      return "nova_diff_drive_controller";
    default:
      return "unknown_controller";
    }
  }

  inline int controlModeToDriveMode(const ControlMode mode)
  {
    switch (mode)
    {
    case ControlMode::PIVOT_DRIVE:
      return nova_interfaces::msg::DriveInfo::PIVOT;
    case ControlMode::STRAFE_DRIVE:
      return nova_interfaces::msg::DriveInfo::STRAFE;
    case ControlMode::DIFF_DRIVE:
      return nova_interfaces::msg::DriveInfo::TANK;
    default:
      return nova_interfaces::msg::DriveInfo::PIVOT; // Default to Pivot
    }
  }

  /**
   * @brief Struct representing the current state.
   */
  struct State
  {
    bool locked = true;
    bool autonomous_mode = false;
    int32_t drive_mode = nova_interfaces::msg::DriveInfo::PIVOT;
  };

  /**
   * @class TeleopDriveJoy
   * @brief Class for handling joystick input and publishing drive commands.
   */
  class TeleopDriveJoy : public rclcpp::Node
  {
  public:
    /**
     * @brief Constructor for TeleopDriveJoy.
     * @param options Node options for the ROS2 node.
     */
    explicit TeleopDriveJoy(const rclcpp::NodeOptions &options = rclcpp::NodeOptions());

    void initializeParams();

  private:
    /**
     * @brief Callback function for joystick messages.
     * @param joy_msg Shared pointer to the joystick message.
     */
    void joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg);

    /**
     * @brief Sends a Drive Command based on joystick input.
     * @param joy_msg Shared pointer to the joystick message.
     */
    void sendDriveCommand(const sensor_msgs::msg::Joy::SharedPtr joy_msg);

    /**
     * @brief Sends a halt command to stop the rover.
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

    void setEnableTwistCmdForController(const std::shared_ptr<rclcpp::Client<rcl_interfaces::srv::SetParameters>> &client, bool enable);

    // Member variables
    rclcpp::Publisher<nova_interfaces::msg::DriveInputStamped>::SharedPtr drive_input_pub;
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_vel_pub;
    rclcpp::Publisher<nova_interfaces::msg::DriveInfo>::SharedPtr drive_info_pub;
    rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub;
    rclcpp::Client<controller_manager_msgs::srv::SwitchController>::SharedPtr switch_controller_client;
    rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr pivot_drive_client;
    rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr strafe_client;
    rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr nova_diff_drive_client;
    std::shared_ptr<ParamListener> param_listener_;

    Params params_;
    bool sent_lock_msg = false;
    State current_state;
    State previous_state;
    ControlMode control_mode = ControlMode::PIVOT_DRIVE;
    double speed; // Linear Speed Multiplier that can be incremented
    std::map<int, rclcpp::Time> last_button_press_time_;
  };

} // namespace teleop_drive_joy

#endif // TELEOP_DRIVE_JOY_HPP