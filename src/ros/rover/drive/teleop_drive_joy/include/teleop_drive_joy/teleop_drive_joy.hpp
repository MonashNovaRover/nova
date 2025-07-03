/**
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * DESCRIPTION: Convert joystick input into drive or twist messages
 * to be received by controllers (pivot, strafe, etc).
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * NODE: teleop_drive_joy
 * TOPICS:
 *  - subscriber: /joy               [sensor_msgs/msg/Joy]
 *  - publisher:  /drive_input       [nova_interfaces/msg/DriveInputStamped]
 *  - publisher:  /cmd_vel           [geometry_msgs/msg/TwistStamped]
 *  - publisher:  /drive_info        [nova_interfaces/msg/DriveInfo]
 * SERVICES:
 *  - client:     /controller_manager/switch_controller        [controller_manager_msgs/srv/SwitchController]
 *  - client:     /pivot_drive_controller/set_parameters       [rcl_interfaces/srv/SetParameters]
 *  - client:     /strafe_controller/set_parameters            [rcl_interfaces/srv/SetParameters]
 *  - client:     /nova_diff_drive_controller/set_parameters   [rcl_interfaces/srv/SetParameters]
 * ACTIONS: None
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * PACKAGE:    teleop_drive_joy
 * AUTHORS:	   Kabi, Terry Tian
 * CREATION:	 ?
 * EDITED:		 15/06/2025
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

 #ifndef TELEOP_DRIVE_JOY_HPP
#define TELEOP_DRIVE_JOY_HPP

#include <cstddef>
#include <map>
#include <functional>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
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
    case nova_interfaces::msg::DriveInfo::HOLONOMIC:
      return "Holonomic Mode";
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
    case nova_interfaces::msg::DriveInfo::HOLONOMIC:
      return "holonomic_drive_controller";
    case nova_interfaces::msg::DriveInfo::STRAFE:
      return "strafe_controller";
    case nova_interfaces::msg::DriveInfo::DIFF:
      return "nova_diff_drive_controller";
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

    // Member variables
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_vel_pub_;
    rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub_;
    rclcpp::Client<controller_manager_msgs::srv::SwitchController>::SharedPtr switch_controller_client_;
    rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr pivot_drive_client_;
    rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr strafe_client_;
    rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr nova_diff_drive_client_;
    std::shared_ptr<ParamListener> param_listener_;

    Params params_;
    bool sent_lock_msg_;
    bool locked_;
    uint drive_mode_;
    double speed_; // Linear Speed Multiplier that can be incremented
    std::map<int, rclcpp::Time> last_button_press_time_;
    std::map<int, std::function<void(const sensor_msgs::msg::Joy::SharedPtr)>> button_callbacks_;
  };

} // namespace teleop_drive_joy

#endif // TELEOP_DRIVE_JOY_HPP