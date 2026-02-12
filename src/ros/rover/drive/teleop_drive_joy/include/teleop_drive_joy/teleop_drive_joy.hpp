/**
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * DESCRIPTION: Convert joystick input into drive or twist messages
 * to be received by controllers (pivot, strafe, etc).
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * NODE: teleop_drive_joy
 * TOPICS:
 *  - subscriber: /joy      [sensor_msgs/msg/Joy]
 *  - publisher:  /cmd_vel  [geometry_msgs/msg/TwistStamped]
 * SERVICES:
 *  - client:     /controller_manager/switch_controller
 * [controller_manager_msgs/srv/SwitchController]
 *  - client:     /pivot_drive_controller/set_parameters   [rcl_interfaces/srv/SetParameters]
 *  - client:     /strafe_drive_controller/set_parameters  [rcl_interfaces/srv/SetParameters]
 *  - client:     /diff_drive_controller/set_parameters    [rcl_interfaces/srv/SetParameters]
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * PACKAGE:   teleop_drive_joy
 * AUTHORS:	  Kabi, Terry Tian
 * CREATION:  2024
 * EDITED:    2025
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef TELEOP_DRIVE_JOY_HPP
#define TELEOP_DRIVE_JOY_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <functional>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <rcl_interfaces/srv/set_parameters.hpp>
#include <controller_manager_msgs/srv/switch_controller.hpp>

#include "teleop_drive_joy_parameters.hpp"

namespace teleop_drive_joy
{

enum class DriveMode : uint8_t
{
  PIVOT,
  ACKERMANN,
  STRAFE,
  DIFF
};

inline std::string pretty_print_mode(const DriveMode mode)
{
  switch (mode)
  {
    case DriveMode::PIVOT:
      return "Pivot mode";
    case DriveMode::ACKERMANN:
      return "Ackermann mode";
    case DriveMode::STRAFE:
      return "Strafe mode";
    case DriveMode::DIFF:
      return "Tank mode";
    default:
      return "Unknown mode";
  }
}

inline std::string mode_to_controller(const DriveMode mode)
{
  switch (mode)
  {
    case DriveMode::PIVOT:
      return "pivot_drive_controller";
    case DriveMode::ACKERMANN:
      return "ackermann_steering_controller";
    case DriveMode::STRAFE:
      return "strafe_drive_controller";
    case DriveMode::DIFF:
      return "diff_drive_controller";
    default:
      return "unknown_controller";
  }
}

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
  explicit TeleopDriveJoy(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

  /**
   * @brief Initializes the TeleopDriveJoy node.
   * This is needed because ParamListener can only be initialized after the node is created
   * (constructor has finished).
   */
  void initialize();

private:
  /**
   * @brief Snaps joystick axes using the max input threshold.
   * @param joy_msg Shared pointer to the joystick message.
   * @return A pair containing the snapped linear and angular axes.
   */
  std::pair<std::pair<double, double>, double> snapped_joy_axes(
    const sensor_msgs::msg::Joy::SharedPtr joy_msg);

  /**
   * @brief Sets the autonomous mode for all controllers.
   * @param autonomous_mode Boolean indicating whether to set autonomous mode.
   */
  void set_autonomous_mode_for_controllers(bool autonomous_mode);

  /**
   * @brief Initializes parameters for the TeleopDriveJoy node.
   */
  void initialize_params();

  /**
   * @brief Initializes the ros2 interfaces for the TeleopDriveJoy node.
   */
  void initialize_interfaces();

  /**
   * @brief Map buttons to their respective callback functions.
   */
  void map_button_callbacks();

  /**
   * @brief Callback function for joystick messages.
   * @param joy_msg Shared pointer to the joystick message.
   */
  void joy_callback(const sensor_msgs::msg::Joy::SharedPtr joy_msg);

  /**
   * @brief Handles button callbacks.
   * @param joy_msg Shared pointer to the joystick message.
   */
  void handle_button_callbacks(const sensor_msgs::msg::Joy::SharedPtr joy_msg);

  /**
   * @brief Sends a Drive Command based on joystick input.
   * @param joy_msg Shared pointer to the joystick message.
   */
  void send_drive_command(const sensor_msgs::msg::Joy::SharedPtr joy_msg);

  /**
   * @brief Sends a halt command to stop the rover.
   */
  void send_halt_command();

  /**
   * @brief Switches the controller by calling the switch_controller service.
   * @param requested_control_mode The desired control mode to switch to.
   */
  void switch_controller(const DriveMode requested_control_mode);

  /**
   * @brief Prints the control mappings to the console.
   */
  void print_controls();

  // Member variables
  rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_vel_pub_;
  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub_;
  rclcpp::Client<controller_manager_msgs::srv::SwitchController>::SharedPtr
    switch_controller_client_;
  rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr pivot_drive_client_;
  rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr strafe_client_;
  rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr diff_drive_client_;
  std::shared_ptr<ParamListener> param_listener_;

  Params params_;
  bool sent_lock_msg_;
  bool locked_;
  DriveMode drive_mode_;
  double speed_;  // Linear Speed Multiplier that can be incremented
  std::map<int, rclcpp::Time> last_button_press_time_;
  std::map<int, std::function<void(const sensor_msgs::msg::Joy::SharedPtr)>> button_callbacks_;
  bool handbrake_pressed_;
};

}  // namespace teleop_drive_joy

#endif  // TELEOP_DRIVE_JOY_HPP