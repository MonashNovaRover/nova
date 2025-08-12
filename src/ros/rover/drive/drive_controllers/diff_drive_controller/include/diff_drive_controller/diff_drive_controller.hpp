#ifndef DIFF_DRIVE_CONTROLLER__DIFF_DRIVE_CONTROLLER_HPP_
#define DIFF_DRIVE_CONTROLLER__DIFF_DRIVE_CONTROLLER_HPP_

#include <chrono>
#include <cmath>
#include <memory>
#include <deque>
#include <string>
#include <vector>

#include "controller_interface/controller_interface.hpp"
#include "hardware_interface/handle.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_buffer.h"
#include "realtime_tools/realtime_publisher.h"
#include "tf2_msgs/msg/tf_message.hpp"

#include "nova_controller_common/speed_limiter.hpp"
#include "nova_controller_common/hardware_interface_wrapper.hpp"
#include "nova_drive_controller_base/nova_drive_controller_base.hpp"
#include "nova_drive_controller_base/odometry.hpp"
#include "diff_drive_controller_parameters.hpp"

namespace diff_drive_controller
{

class DiffDriveController : public nova_drive_controller_base::NovaDriveControllerBase
{
public:
  DiffDriveController();

protected:
  void init_params() override;
  void update_params() override;
  void reset_limiter_buffers() override;
  nova_drive_controller_base::Commands twist_to_commands(
    const geometry_msgs::msg::Twist& twist_msg, bool autonomous_mode,
    const rclcpp::Duration& period) override;

  // Parameters from ROS for diff_drive_controller
  std::shared_ptr<ParamListener> param_listener_;
  Params params_;

  double wheel_separation_;
  double left_wheel_radius_;
  double right_wheel_radius_;

  std::deque<double> previous_speeds_;              // last two speed commands
  std::deque<double> previous_angular_velocities_;  // last two angular velocity commands
};

}  // namespace diff_drive_controller

#endif  // DIFF_DRIVE_CONTROLLER__DIFF_DRIVE_CONTROLLER_HPP_
