#ifndef NOVA_ARM_CONTROLLER__SELF_COLLISION_LIMITER_HPP_
#define NOVA_ARM_CONTROLLER__SELF_COLLISION_LIMITER_HPP_

#include "arm_kinematics/collision/collision_manager.hpp"
#include "rclcpp/logger.hpp"
#include "rclcpp/duration.hpp"
#include "trajectory_msgs/msg/joint_trajectory_point.hpp"
#include <cstddef>

namespace nova_arm_controller
{

class SelfCollisionLimiter
{
public:
  SelfCollisionLimiter() = default;

  void set_collision_manager(
    arm_kinematics::CollisionManager collision_manager,
    rclcpp::Logger logger,
    size_t joint_count);

  // Returns true if collision was detected (desired state was reverted).
  bool enforce(
    const trajectory_msgs::msg::JointTrajectoryPoint & current,
    trajectory_msgs::msg::JointTrajectoryPoint & desired,
    const rclcpp::Duration & dt);

private:
  arm_kinematics::CollisionManager collision_manager_;
  rclcpp::Logger logger_ = rclcpp::get_logger("SelfCollisionLimiter");
  size_t joint_count_{0};
};

} // namespace nova_arm_controller
#endif // NOVA_ARM_CONTROLLER__SELF_COLLISION_LIMITER_HPP_
