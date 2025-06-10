/**
 * @file teleop_drive_joy.hpp
 * @brief Header file for the TeleopDriveJoy class, which handles joystick input for teleoperation.
 * Last Edited by Kabi
 */
#ifndef TELEOP_DRIVE_JOY_HPP
#define TELEOP_DRIVE_JOY_HPP

#include <cstddef>
#include <map>
#include <functional>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <nova_interfaces/msg/drive_input_stamped.hpp>
#include <nova_interfaces/msg/drive_info.hpp>
#include <controller_manager_msgs/srv/switch_controller.hpp>

#include "teleop_drive_joy_parameters.hpp"

namespace teleop_drive_joy
{

  inline std::string prettyPrintMode(const uint mode)
  {
    switch (mode)
    {
    case nova_interfaces::msg::DriveInfo::PIVOT:
      return "Pivot Mode";
    case nova_interfaces::msg::DriveInfo::STRAFE:
      return "Strafe Mode";
    case nova_interfaces::msg::DriveInfo::DIFF:
      return "Tank Mode";
    default:
      return "Unknown Mode";
    }
  }

  inline std::string modeToController(const uint mode)
  {
    switch (mode)
    {
    case nova_interfaces::msg::DriveInfo::PIVOT:
      return "pivot_drive_controller";
    case nova_interfaces::msg::DriveInfo::STRAFE:
      return "strafe_controller";
    case nova_interfaces::msg::DriveInfo::DIFF:
      return "nova_diff_drive_controller";
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
    bool autonomous_mode = false;
    uint drive_mode = nova_interfaces::msg::DriveInfo::PIVOT;
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
    
    /**
     * @brief Initializes the TeleopDriveJoy node.
     * This is needed because ParamListener can only be initialized after the node is created (constructor has finished).
     */
    void initialize();

  private:
    /**
     * @brief Initializes parameters for the TeleopDriveJoy node.
     */
    void initializeParams();

    /**
     * @brief Initializes the ros2 interfaces for the TeleopDriveJoy node.
     */
    void initializeInterfaces();

    /**
     * @brief Map buttons to their respective callback functions.
     */
    void mapButtonCallbacks();
    
    /**
     * @brief Callback function for joystick messages.
     * @param joy_msg Shared pointer to the joystick message.
     */
    void joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg);

    /**
     * @brief Handles button callbacks.
     * @param joy_msg Shared pointer to the joystick message.
     */
    void handleButtonCallbacks(const sensor_msgs::msg::Joy::SharedPtr joy_msg);

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
     * @brief Switches the controller by calling the switch_controller service.
     * @param requested_control_mode The desired control mode to switch to.
     */
    void switchController(const uint requested_control_mode);

    void setEnableTwistCmdForController(const std::shared_ptr<rclcpp::Client<rcl_interfaces::srv::SetParameters>> &client, bool enable);

    // Member variables
    rclcpp::Publisher<nova_interfaces::msg::DriveInputStamped>::SharedPtr drive_input_pub_;
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_vel_pub_;
    rclcpp::Publisher<nova_interfaces::msg::DriveInfo>::SharedPtr drive_info_pub_;
    rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub_;
    rclcpp::Client<controller_manager_msgs::srv::SwitchController>::SharedPtr switch_controller_client_;
    rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr pivot_drive_client_;
    rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr strafe_client_;
    rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr nova_diff_drive_client_;
    std::shared_ptr<ParamListener> param_listener_;

    Params params_;
    bool sent_lock_msg_ = false;
    State current_state_;
    State previous_state_;
    uint control_mode_ = nova_interfaces::msg::DriveInfo::PIVOT;
    double speed_; // Linear Speed Multiplier that can be incremented
    std::map<int, rclcpp::Time> last_button_press_time_;
    std::map<int, std::function<void(const sensor_msgs::msg::Joy::SharedPtr)>> button_callbacks_;
  };

} // namespace teleop_drive_joy

#endif // TELEOP_DRIVE_JOY_HPP