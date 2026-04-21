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
 * AUTHORS:	  Kabi, Terry Tian, Jonathan Jia
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
#include <optional>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <sensor_msgs/msg/joy_feedback.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <rcl_interfaces/srv/set_parameters.hpp>
#include <controller_manager_msgs/srv/switch_controller.hpp>
#include <drive_interfaces/msg/drive_info.hpp>
#include <blcmd_interfaces/msg/blcmd_log.hpp>

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

inline uint8_t mode_to_drive_info(const DriveMode mode)
{
  switch (mode)
  {
  case DriveMode::PIVOT:
    return drive_interfaces::msg::DriveInfo::PIVOT;
  case DriveMode::ACKERMANN:
    return drive_interfaces::msg::DriveInfo::ACKERMANN;
  case DriveMode::STRAFE:
    return drive_interfaces::msg::DriveInfo::STRAFE;
  case DriveMode::DIFF:
    return drive_interfaces::msg::DriveInfo::TANK;
  default:
    return std::numeric_limits<uint8_t>::max();
  }
}

struct AxisCallback
{
  // callback should run if axis value is between start and end (both inclusive)
  // (if any empty, then inf and -inf respectively)
  // note: current implementation means, for each axis in axis_callbacks_,
  // only the first callback which has current axis value in [start, end] is called
  std::optional<double> start;
  std::optional<double> end;

  std::function<void(const sensor_msgs::msg::Joy::SharedPtr)> callback;
};

class RumbleCalculator
{
public:
  RumbleCalculator(std::vector<double> continuous_from_effort_range,
    std::vector<double> continuous_to_intensity_range,
    std::chrono::milliseconds continuous_rumble_timeout,
    double timeout_intensity_change_per_second,
    std::vector<double> transient_from_effort_change_range,
    std::vector<double> transient_to_intensity_range,
    bool continuous_rumble_timeout_enable = true)
  : continuous_rumble_timeout_enable(continuous_rumble_timeout_enable),
  continuous_from_effort_range(std::move(continuous_from_effort_range)),
  continuous_to_intensity_range(std::move(continuous_to_intensity_range)),
  continuous_rumble_timeout(continuous_rumble_timeout),
  timeout_intensity_change_per_second(timeout_intensity_change_per_second),
  transient_from_effort_change_range(std::move(transient_from_effort_change_range)),
  transient_to_intensity_range(std::move(transient_to_intensity_range))
  { }

  void update(double effort, rclcpp::Time now);

  double transient_rumble() const;

  double continuous_rumble() const;

  double rumble() const
  {
    return std::max(transient_rumble(), continuous_rumble());
  }

private:

  template <typename T>
  static T linear_interpolation(T value, const std::vector<T>& from, const std::vector<T>& to)
  {
    auto clamped_value = std::clamp(value, from[0], from[1]);
    return ((clamped_value - from[0]) / (from[1] - from[0])) * (to[1] - to[0]) + to[0];
  }

  bool continuous_rumble_active() const
  {
    return std::abs(effort_history.value().back()) > continuous_from_effort_range[0];
  }

  int history_depth {2};
  std::optional<std::vector<double>> effort_history;
  std::optional<std::vector<rclcpp::Time>> update_history;
  std::optional<rclcpp::Time> start_continuous_rumble;
  double timeout_rumble_intensity_adjustment {0};

  // constants for continuous rumble intensity calculations

  // effort (in continuous_from_effort_range) is mapped to intensity (in continuous_to_intensity_range)
  std::vector<double> continuous_from_effort_range;
  std::vector<double> continuous_to_intensity_range;

  // constants for timeout to continuous rumbling
  bool continuous_rumble_timeout_enable {};
  std::chrono::milliseconds continuous_rumble_timeout;
  double timeout_intensity_change_per_second;

  // constants for transient rumble intensity calculations
  std::vector<double> transient_from_effort_change_range; // where effort change is in "effort per second"
  std::vector<double> transient_to_intensity_range;
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
   * @brief Updates parameters for the TeleopDriveJoy node.
   */
  void update_params();

  /**
   * @brief Initializes the ros2 interfaces for the TeleopDriveJoy node.
   */
  void initialize_interfaces();

  /**
   * @brief Initializes gamepad rumble according to parameters
   */
  void initialize_gamepad_rumble();

  /**
   * @brief Map buttons to their respective callback functions.
   */
  void map_button_callbacks();

  /**
   * @brief Callback function for main publish loop timer.
   */
  void timer_callback();
  
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
   * @brief Handles axis callbacks.
   * @param joy_msg Shared pointer to the joystick message.
   */
  void handle_axis_callbacks(const sensor_msgs::msg::Joy::SharedPtr joy_msg);

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

  /**
   * @brief Updates connection status to game pad
   * @param connected Whether game pad is connected or not
   */
  void set_connected(bool connected);

  /**
   * @brief Sends current state of teleop drive joy as drive info
   */
  void send_drive_info();

    /**
   * @brief Callback function for joint state messages.
   * @param joint_state_msg Shared pointer to the joint state message.
   */
  void joint_states_callback(const sensor_msgs::msg::JointState::SharedPtr joint_state_msg);

  /**
  * @brief Callback function for blcmd log messages.
  * @param blcmd_log_msg Shared pointer to the log message.
  */
  void blcmd_log_callback(const blcmd_interfaces::msg::BLCMDLog::SharedPtr blcmd_log_msg);

  /**
  * @brief Activates lock if any autolock threshold has been breached
  */
  void apply_autolock();

  // Member variables
  rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_vel_pub_;
  rclcpp::Publisher<drive_interfaces::msg::DriveInfo>::SharedPtr drive_info_pub_;
  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub_;
  rclcpp::Publisher<sensor_msgs::msg::JoyFeedback>::SharedPtr joy_feedback_pub_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_state_sub_;
  rclcpp::Subscription<blcmd_interfaces::msg::BLCMDLog>::SharedPtr blcmd_log_sub_;
  rclcpp::TimerBase::SharedPtr timer_;

  rclcpp::Client<controller_manager_msgs::srv::SwitchController>::SharedPtr
    switch_controller_client_;

  rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr pivot_drive_client_;
  rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr strafe_client_;
  rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr diff_drive_client_;

  std::shared_ptr<ParamListener> param_listener_;
  rclcpp::TimerBase::SharedPtr connection_timer_;

  std::map<int, int> blcmd_error_count_ {};
  std::map<int, rclcpp::Time> blcmd_times_start_error_ {};
  std::map<int, rclcpp::Time> blcmd_times_last_error_ {};
  bool autolock_override_trigger;

  Params params_;
  sensor_msgs::msg::Joy::SharedPtr joy_msg_;
  bool locked_;
  uint8_t locked_reason_;
  DriveMode drive_mode_;
  double speed_;  // Linear Speed Multiplier that can be incremented
  std::map<int, rclcpp::Time> last_button_press_time_;
  std::map<int, std::function<void(const sensor_msgs::msg::Joy::SharedPtr)>> button_callbacks_;
  std::map<int, std::vector<AxisCallback>> axis_callbacks_;
  bool handbrake_pressed_;
  bool autonomous_mode_;
  bool connected_;

  std::map<std::string, RumbleCalculator> rumble_calculators;
};

}  // namespace teleop_drive_joy

#endif  // TELEOP_DRIVE_JOY_HPP