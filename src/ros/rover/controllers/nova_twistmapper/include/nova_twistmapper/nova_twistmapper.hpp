#ifndef NOVA_TWISTMAPPER__NOVA_TWISTMAPPER_HPP_
#define NOVA_TWISTMAPPER__NOVA_TWISTMAPPER_HPP_

#include <chrono>
#include <cmath>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "visibility_control.h"
#include "controller_interface/controller_interface.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "hardware_interface/handle.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "rclcpp/node.hpp"
#include "realtime_tools/realtime_box.h"
//#include "realtime_tools/realtime_buffer.h"
//#include "realtime_tools/realtime_publisher.h"
#include "tf2_msgs/msg/tf_message.hpp"
#include "tf2/LinearMath/Scalar.h"
#include <Eigen/Dense>
#include <tf2/LinearMath/Transform.h>
#include <tf2_ros/transform_broadcaster.h>
#include "tf2_ros/transform_listener.h"
#include "tf2_ros/buffer.h"
#include "PoseHandle.hpp"

// To test in development, run from the root nova_twistmapper dir:
// generate_parameter_library_cpp include/nova_twistmapper/nova_twistmapper_parameters.hpp src/nova_twistmapper_parameter.yaml
#include "nova_twistmapper_parameters.hpp"

namespace nova_twistmapper
{
class NovaTwistmapper : public controller_interface::ControllerInterface
{
public:
  NovaTwistmapper();

  controller_interface::InterfaceConfiguration command_interface_configuration() const override;

  controller_interface::InterfaceConfiguration state_interface_configuration() const override;

  controller_interface::return_type update(
    const rclcpp::Time &time, const rclcpp::Duration &period) override;

  controller_interface::CallbackReturn on_init() override;

  controller_interface::CallbackReturn on_configure(
    const rclcpp_lifecycle::State &previous_state) override;

  controller_interface::CallbackReturn on_activate(
    const rclcpp_lifecycle::State &previous_state) override;

  controller_interface::CallbackReturn on_deactivate(
    const rclcpp_lifecycle::State &previous_state) override;

  controller_interface::CallbackReturn on_cleanup(
    const rclcpp_lifecycle::State &previous_state) override;

  controller_interface::CallbackReturn on_error(
    const rclcpp_lifecycle::State &previous_state) override;

  controller_interface::CallbackReturn on_shutdown(
    const rclcpp_lifecycle::State &previous_state) override;

protected:
  // Holds command interfaces for different components of the pose
  std::optional<PoseHandle> pose_handle;

  // Helpers
  std::string pose_component_to_command_interface_name(const std::string& component_name) const;

  // Parameters from ROS for nova_diff_drive_controller
  std::shared_ptr<ParamListener> param_listener_;
  Params params_;

  // Twistmapper
  rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr twist_stamped_sub = nullptr;
  realtime_tools::RealtimeBox<std::shared_ptr<geometry_msgs::msg::TwistStamped>> received_twist_stamped_ptr{nullptr};

  /// Result of the twistmapper, and input to IK. Desired position and orientation of the end effector relative to the base.
  tf2::Transform twistmapper_pose_ = tf2::Transform();
  tf2::Vector3 twistmapper_pose_rpy_ = tf2::Vector3();
  rclcpp::Time twistmapper_pose_update_time_ = rclcpp::Time();
  /// True when initial state has been received to confirm that the value in _twistmapper_pose is safe and valid

  // broadcasting twistmapper
  std::shared_ptr<tf2_ros::TransformBroadcaster> twistmapper_pose_tf_broadcaster_;
  // FK for getting the initial state for twistmapper_pose_
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_{nullptr};
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;

  void update_twistmapper_pose(const rclcpp::Time &time, const rclcpp::Duration &period);

  // Timeout to consider cmd_vel commands old
  std::chrono::milliseconds cmd_vel_timeout_{500};
  bool subscriber_is_active_ = false; // not sure what this is for yet
  rclcpp::Time previous_update_timestamp_{0};

  // publish rate limiter
  // double publish_rate_ = 50.0;
  // rclcpp::Duration publish_period_ = rclcpp::Duration::from_nanoseconds(0);
  // rclcpp::Time previous_publish_timestamp_{0, 0, RCL_CLOCK_UNINITIALIZED};
  bool is_halted = false;
  bool reset();
  void halt();

  void publish_to_tf2(const rclcpp::Time &time);
};
} // namespace nova_twistmapper
#endif // NOVA_TWISTMAPPER__NOVA_TWISTMAPPER_HPP_
