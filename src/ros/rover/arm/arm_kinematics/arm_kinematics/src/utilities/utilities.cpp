//
// Created by Bailey Chessum on 14/10/2025.
//

#include <arm_kinematics/utilities/utilities.hpp>

namespace
{
// Used to avoid division by zero. Threshold for where to call small numbers essentially zero in vector normalization.
constexpr auto EPSILON = 1e-8;
} // namespace

namespace arm_kinematics {

void apply_twist(const Eigen::Matrix<double, 6, 1> &twist, const double delta_time,
                                 const Eigen::Isometry3d &pose, Eigen::Isometry3d &result) {
  Eigen::Vector3d twist_linear = twist.block<3, 1>(0, 0);
  Eigen::Vector3d twist_angular = twist.block<3, 1>(3, 0);

  // Set translational components
  result.translation() = pose.translation() + twist_linear * delta_time;

  // Set angular components
  auto twist_angular_norm = twist_angular.norm();

  // TODO: See if you can do this better for real time safety
  // Only apply rotation if it is non-zero enough to avoid precision errors
  if (twist_angular_norm > EPSILON) {
    // Create rotation matrix from twist_angular * period.seconds. This isn't an angular velocity, but a displacement.
    Eigen::AngleAxisd angular_diff(twist_angular_norm * delta_time, twist_angular / twist_angular_norm);

    // This 'linear' does not mean the same thing as the twist's 'linear'!
    // It is the linear component of the affine transformation matrix.
    result.linear() = angular_diff.toRotationMatrix() * pose.linear();
  }
  else {
    result.linear() = pose.linear();
  }
}

Eigen::Isometry3d apply_twist(const Eigen::Matrix<double, 6, 1> &twist, const double delta_time,
                                              const Eigen::Isometry3d &pose) {
  Eigen::Isometry3d result;
  apply_twist(twist, delta_time, pose, result);
  return result;
}

void kdl_to_eigen(const KDL::Frame & frame, Eigen::Isometry3d & result) {
  // Rotation
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      result.linear()(i, j) = frame.M(i, j);

  // Translation
  result.translation() << frame.p.x(), frame.p.y(), frame.p.z();
}

Eigen::Isometry3d kdl_to_eigen(const KDL::Frame & frame) {
  Eigen::Isometry3d result{};
  kdl_to_eigen(frame, result);
  return result;
}

}  // namespace arm_kinematics
