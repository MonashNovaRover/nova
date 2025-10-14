//
// Created by Bailey Chessum on 14/10/2025.
//

#include <arm_kinematics/kinematics_plugin_base.hpp>

#include <urdf/model.h>
#include <stdexcept>

namespace
{
// Used to avoid division by zero. Threshold for where to call small numbers essentially zero in vector normalization.
constexpr auto EPSILON = 1e-8;
} // namespace

namespace arm_kinematics {

bool KinematicsPluginBase::initialize(KinematicsPluginBase::KinematicsNodeInterfaces node_interfaces,
                                      std::string &robot_description, const std::vector<std::string> &joint_names) {
  robot_description_ = &robot_description;
  joint_names_ = joint_names;

  node_interfaces_ = node_interfaces;
  logger_ = node_interfaces.get_node_logging_interface()->get_logger().get_child("kinematics");

  return on_initialize();
}

bool KinematicsPluginBase::get_position_fk(const std::vector<double> &joint_angles,
                                           const std::string &link_name,
                                           Eigen::Isometry3d &solution_pose) const {
  // Default implementation using KDL
  return false;
}

bool KinematicsPluginBase::get_position_fk(const std::vector<double> &joint_angles,
                                           Eigen::Isometry3d &solution_pose) const {
  // Default implementation just uses the general case implementation
  // But you could override this if you have an analytical solution

  return get_position_fk(joint_angles, get_kinematics_params().ee_link_name, solution_pose);
}

bool KinematicsPluginBase::get_velocity_ik(const Eigen::Matrix<double, 6, 1> &ik_twist,
                                           const Eigen::Isometry3d &ik_seed_pose,
                                           const std::vector<double> &ik_seed_state,
                                           std::vector<double> &solution_velocities,
                                           double time_step) const
{
  // TODO: Make helper function to apply a twist to an Isometry3D
  // Apply the ik_twist over time_step to get another pose
  Eigen::Vector3d twist_linear = ik_twist.block<3, 1>(0, 0);
  Eigen::Vector3d twist_angular = ik_twist.block<3, 1>(3, 0);

  // Construct a 4x4 matrix from the above linear and angular values, but as a displacement rather than velocity.
  Eigen::Isometry3d new_pose = ik_seed_pose;

  // Set translational components
  new_pose.translation() = ik_seed_pose.translation() + twist_linear * time_step;

  // Set angular components
  auto twist_angular_norm = twist_angular.norm();
  // TODO: See if you can do this better for real time safety
  // Only apply rotation if it is non-zero enough to avoid precision errors
  if (twist_angular_norm > EPSILON) {
    // Create rotation matrix from twist_angular * period.seconds. This isn't an angular velocity, but a displacement.
    Eigen::AngleAxisd angular_diff(twist_angular_norm * time_step, twist_angular / twist_angular_norm);

    // This 'linear' does not mean the same thing as the twist's 'linear'!
    // It is the linear component of the affine transformation matrix.
    new_pose.linear() = angular_diff.toRotationMatrix() * ik_seed_pose.linear();
  }

  // Get the difference of the resulting joint angles for the time_step to get vel.
  // solution_velocities will contain twist
  auto ik_result = get_position_ik(new_pose, ik_seed_state, solution_velocities);
}

bool KinematicsPluginBase::get_velocity_ik(const Eigen::Matrix<double, 6, 1> &ik_twist,
                                           const std::vector<double> &ik_seed_state,
                                           std::vector<double> &solution_velocities,
                                           double time_step) const
{
  // Just call the above method, getting ik_seed_pose from forward kinematics
  Eigen::Isometry3d ik_seed_pose;
  const auto fk_result = get_position_fk(ik_seed_state, ik_seed_pose);

  return fk_result && get_velocity_ik(ik_twist, ik_seed_pose, ik_seed_state, solution_velocities, time_step);
}

const std::vector<std::string> &KinematicsPluginBase::get_joint_names() const noexcept {
  return joint_names_;
}

const std::string & KinematicsPluginBase::get_robot_description() const {
  if (!robot_description_)
    throw std::logic_error("Tried to use a KinematicsPlugin before calling initialize() or after initialize() failed.");

  return *robot_description_;
}


const rclcpp::Logger &KinematicsPluginBase::get_logger() const noexcept {
  return logger_;
}

const KinematicsParams &KinematicsPluginBase::get_kinematics_params() const noexcept {
  return kinematics_params_;
}

const KinematicsPluginBase::KinematicsNodeInterfaces & KinematicsPluginBase::get_node_interfaces() const {
  if (!node_interfaces_.has_value())
    throw std::logic_error("Tried to use a KinematicsPlugin before calling initialize() or after initialize() failed.");

  return *node_interfaces_;
}


} // arm_kinematics