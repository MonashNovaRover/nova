//
// Created by Bailey Chessum on 14/10/2025.
//

#ifndef ARM_KINEMATICS_UTILITIES_HPP
#define ARM_KINEMATICS_UTILITIES_HPP

#include <Eigen/Geometry>
#include <kdl/frames.hpp>

namespace arm_kinematics {

  /**
   * Applies a given twist to a pose.
   *
   * \param[in] twist The twist to be applied. twist.block<3, 1>(0, 0) is the linear component, and
   *                  twist.block<3, 1>(3, 0) is the angular component.
   * \param[in] pose The pose to apply the twist to
   * \param[out] result The pose with the twist applied
   */
  void apply_twist(const Eigen::Matrix<double, 6, 1> & twist, double delta_time, const Eigen::Isometry3d & pose,
                   Eigen::Isometry3d & result);

  /**
   * Applies a given twist to a pose.
   *
   * \param[in] twist The twist to be applied. twist.block<3, 1>(0, 0) is the linear component, and
   *                  twist.block<3, 1>(3, 0) is the angular component.
   * \param[in] pose The pose to apply the twist to
   * \returns The pose with the twist applied
   */
  Eigen::Isometry3d apply_twist(const Eigen::Matrix<double, 6, 1> & twist, double delta_time, const Eigen::Isometry3d & pose);

  /**
   * Converts a KDL::Frame to an Eigen::Isometry3d
   *
   * \param[in] frame The value to convert to an Eigen::Isometry3d
   * \param[out] result frame as an Eigen::Isometry3d
   */
  void kdl_to_eigen(const KDL::Frame & frame, Eigen::Isometry3d & result);

  /**
   * Converts a KDL::Frame to an Eigen::Isometry3d
   *
   * \param[in] frame The value to convert to an Eigen::Isometry3d
   * \returns frame as an Eigen::Isometry3d
   */
  Eigen::Isometry3d kdl_to_eigen(const KDL::Frame & frame);
}

#endif //ARM_KINEMATICS_UTILITIES_HPP
