//
// Created by Bailey Chessum on 14/10/2025.
//

#ifndef ARM_KINEMATICS_UTILITIES_HPP
#define ARM_KINEMATICS_UTILITIES_HPP

#include <Eigen/Geometry>
#include <kdl/frames.hpp>

#include "arm_kinematics/utilities/aliases.hpp"

namespace arm_kinematics {

  /**
   * Applies a given twist to a pose.
   *
   * \param[in] twist The twist to be applied. twist.block<3, 1>(0, 0) is the linear component, and
   *                  twist.block<3, 1>(3, 0) is the angular component.
   * \param[in] pose The pose to apply the twist to
   * \param[out] result The pose with the twist applied
   */
  void apply_twist(const Twistf & twist, double delta_time, const Eigen::Isometry3f & pose,
                   Eigen::Isometry3f & result);

  /**
   * Applies a given twist to a pose.
   *
   * \param[in] twist The twist to be applied. twist.block<3, 1>(0, 0) is the linear component, and
   *                  twist.block<3, 1>(3, 0) is the angular component.
   * \param[in] pose The pose to apply the twist to
   * \returns The pose with the twist applied
   */
  Eigen::Isometry3f apply_twist(const Twistf & twist, double delta_time, const Eigen::Isometry3f & pose);

  /**
   * Converts a KDL::Frame to an Eigen::Isometry3f
   *
   * \param[in] frame The value to convert to an Eigen::Isometry3f
   * \param[out] result frame as an Eigen::Isometry3f
   */
  void kdl_to_eigen(const KDL::Frame & frame, Eigen::Isometry3f & result);

  /**
   * Converts a KDL::Frame to an Eigen::Isometry3f
   *
   * \param[in] frame The value to convert to an Eigen::Isometry3f
   * \returns frame as an Eigen::Isometry3f
   */
  Eigen::Isometry3f kdl_to_eigen(const KDL::Frame & frame);

}

#endif //ARM_KINEMATICS_UTILITIES_HPP
