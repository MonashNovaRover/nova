//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_INVERSE_KINEMATICS_PLUGIN_HPP
#define ARM_KINEMATICS_INVERSE_KINEMATICS_PLUGIN_HPP

#include <vector>
#include <Eigen/Geometry>
#include <memory>

#include "arm_kinematics/visibility_control.h"
#include "arm_kinematics/common/kinematics_base.hpp"
#include "arm_kinematics/utilities/aliases.hpp"

namespace arm_kinematics {

/**
 * Base class for plugins used to perform inverse kinematics.
 *
 * This does not provide a default implementation, as IK implementations are expected to be solved analytically. We have
 * faced reliability issues in the past when trying to use numerical solvers (URC 2025).
 */
class ARM_KINEMATICS_PUBLIC InverseKinematicsPlugin : public KinematicsBase {
public:
  using SharedPtr = std::shared_ptr<InverseKinematicsPlugin>;

  /**
   * Effectively replaces the constructor for the class, as we can only use a default constructor in plugins.
   *
   * \warning Very expensive, and obviously not real-time safe.
   *
   * \returns True if initialization was successful. False otherwise.
   */
  bool initialize(
    const KinematicsNodeInterfaces & node_interfaces,
    const RobotModel & robot_model,
    KinematicsParams::SharedPtr kinematics_params);

  /**
   * Get the positions of all joints given the desired final position of the ee_link.
   *
   * \param ik_pose The desired pose of the end effector, in the reference frame of the base_link.
   * \param ik_seed_state The current joint positions. The returned solution will be the closest valid solution to this
   * seed state.
   * \param[out] solution_state The set of joint positions that would result in the end effector being moved to ik_pose.
   *
   * \note Make sure to pre-allocate vectors outside the real-time loop with the correct number of elements (same
   * number of joints as in joint names).
   *
   * \returns True if a solution was found
   */
  virtual bool get_position_ik(
    const Eigen::Isometry3f & ik_pose,
    const std::vector<double> & ik_seed_state,
    std::vector<double> & solution_state) const = 0;

  /**
   * Estimate the velocities of each joint needed to have the end effector move at some twist.
   *
   * \param[in] ik_twist The twist to get equivalent joint velocities for, in the reference frame of the base_link.
   *   - twist.block<3, 1>(0, 0) is the linear component, and
   *   - twist.block<3, 1>(3, 0) is the angular component.
   *   - You can construct this from a Twist message using tf2_eigen's tf2::from_msg() helper.
   * \param[in] ik_seed_pose The pose of the end effector that matches ik_seed_state. Can be obtained using FK.
   * If ik_seed_pose and ik_seed_state are not an exact match, the given solution will be wrong.
   * \param[in] ik_seed_state The current set of joint positions.
   * \param[out] solution_velocities The velocities of each joint to sustain an average twist in the end effector equal
   * to ik_twist over a period of time_step.
   * \param[in] time_step The change in time used to extrapolate the ik_seed_pose by ik_twist, and estimate the
   * derivative of joint positions. This must be non-zero!!
   *
   * \note Make sure to pre-allocate vectors outside of the real-time loop with the correct number of elements (same
   * number of joints as in joint names).
   *
   * \returns True if a solution was found.
   */
  virtual bool get_velocity_ik(
    const Twistf & ik_twist,
    const Eigen::Isometry3f & ik_seed_pose,
    const std::vector<double> & ik_seed_state,
    std::vector<double> & solution_velocities,
    double time_step) const;

protected:
  /**
   * Called when the kinematics plugin is created. Override this to add any set up logic for the kinematics plugin.
   *
   * \returns True if initialization was successful. False otherwise.
   */
  virtual bool on_initialize() = 0;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_INVERSE_KINEMATICS_PLUGIN_HPP
