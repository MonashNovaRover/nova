#include "nova_arm_controller/self_collision_limiter.hpp"
#include "rclcpp/logging.hpp"

namespace nova_arm_controller
{

void SelfCollisionLimiter::set_collision_manager(
  arm_kinematics::CollisionManager collision_manager,
  rclcpp::Logger logger,
  size_t joint_count)
{
  collision_manager_ = std::move(collision_manager);
  logger_ = logger;
  joint_count_ = joint_count;
}

bool SelfCollisionLimiter::enforce(
  const trajectory_msgs::msg::JointTrajectoryPoint & current,
  trajectory_msgs::msg::JointTrajectoryPoint & desired,
  const rclcpp::Duration & dt)
{
  const auto dt_seconds = dt.seconds();
  if (dt_seconds <= 0.0) {
    return false;
  }

  const bool has_desired_pos = (desired.positions.size() == joint_count_);
  const bool has_desired_vel = (desired.velocities.size() == joint_count_);
  const bool has_current_pos = (current.positions.size() == joint_count_);

  if (!(has_current_pos && (has_desired_pos || has_desired_vel))) {
    return false;
  }

  // Build target position vector in joint_names order (same order as params_.joint_names,
  // which is the order used to construct the CollisionManager).
  std::vector<double> target_positions(joint_count_);
  for (size_t i = 0; i < joint_count_; ++i) {
    target_positions[i] = has_desired_pos
      ? desired.positions[i]
      : current.positions[i] + desired.velocities[i] * dt_seconds;
  }

  collision_manager_.update_poses(target_positions);

  if (!collision_manager_.collide()) {
    return false;
  }

  RCLCPP_WARN(logger_, "Self-collision detected — reverting desired state");
  if (has_desired_pos) {
    desired.positions = current.positions;
  }
  if (has_desired_vel) {
    std::fill(desired.velocities.begin(), desired.velocities.end(), 0.0);
  }
  return true;
}

} // namespace nova_arm_controller
