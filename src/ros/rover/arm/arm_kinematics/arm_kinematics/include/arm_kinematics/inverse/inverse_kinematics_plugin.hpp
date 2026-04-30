//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_INVERSE_KINEMATICS_PLUGIN_HPP
#define ARM_KINEMATICS_INVERSE_KINEMATICS_PLUGIN_HPP

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <Eigen/Geometry>

#include "arm_kinematics/visibility_control.h"
#include "arm_kinematics/common/kinematics_base.hpp"
#include "arm_kinematics/utilities/aliases.hpp"
#include "arm_kinematics/utilities/twist.hpp"
#include "arm_kinematics/utilities/expected.hpp"
#include "arm_kinematics/utilities/span.hpp"

namespace arm_kinematics {

struct ARM_KINEMATICS_PUBLIC IKFailure {
  struct NoSolution {
    std::string detail{};

    [[nodiscard]] std::string format() const;
  };

  struct InvalidSeed {
    std::string detail{};

    [[nodiscard]] std::string format() const;
  };

  struct InvalidTarget {
    std::string detail{};

    [[nodiscard]] std::string format() const;
  };

  struct OutsideWorkspace {
    std::string detail{};

    [[nodiscard]] std::string format() const;
  };

  struct Singular {
    std::string detail{};

    [[nodiscard]] std::string format() const;
  };

  struct BackendError {
    std::string detail{};

    [[nodiscard]] std::string format() const;
  };

  using Variant = std::variant<
    NoSolution,
    InvalidSeed,
    InvalidTarget,
    OutsideWorkspace,
    Singular,
    BackendError>;
  Variant value;

  template <
    typename T,
    typename Decayed = std::decay_t<T>,
    typename = std::enable_if_t<!std::is_same_v<Decayed, IKFailure>>>
  IKFailure(T && t)
  : value(std::forward<T>(t))
  {
  }

  [[nodiscard]] std::string format() const;
};

using IKResult = tl::expected<void, IKFailure>;

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
   * \param[out] solution_state The set of joint positions that would result in the end effector
   * being moved to ik_pose. Callers should pass a pre-sized writable span.
   *
   * \returns Success if a solution was found, or a structured failure otherwise.
   */
  [[nodiscard]] virtual IKResult get_position_ik(
    const Eigen::Isometry3d & ik_pose,
    span<const double> ik_seed_state,
    span<double> solution_state) const = 0;

  /**
   * Estimate the velocities of each joint needed to have the end effector move at some twist.
   *
   * \param[in] ik_twist The twist to get equivalent joint velocities for, in the reference frame of the base_link.
   *   - `ik_twist.linear()` is the linear component, and
   *   - `ik_twist.angular()` is the angular component.
   *   - You can construct this from a Twist message using tf2_eigen's tf2::from_msg() helper.
   * \param[in] ik_seed_pose The pose of the end effector that matches ik_seed_state. Can be obtained using FK.
   * If ik_seed_pose and ik_seed_state are not an exact match, the given solution will be wrong.
   * \param[in] ik_seed_state The current set of joint positions.
   * \param[out] solution_velocities The velocities of each joint to sustain the requested twist.
   * Callers should pass a pre-sized writable span.
   * \param[in] time_step The change in time used to extrapolate the ik_seed_pose by ik_twist, and estimate the
   * derivative of joint positions. This must be non-zero!!
   *
   * \returns Success if a solution was found, or a structured failure otherwise.
   */
  [[nodiscard]] virtual IKResult get_velocity_ik(
    const Twistd & ik_twist,
    const Eigen::Isometry3d & ik_seed_pose,
    span<const double> ik_seed_state,
    span<double> solution_velocities,
    double time_step);

protected:
  /**
   * Called when the kinematics plugin is created. Override this to add any set up logic for the kinematics plugin.
   *
   * \returns True if initialization was successful. False otherwise.
   */
  virtual bool on_initialize() = 0;

  /// Allocated to the size of the joints in the KinematicsParams::SharedPtr used in initialization.
  /// Needed for the get_velocity_ik default implementation.
  std::vector<double> per_joint_scratch_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_INVERSE_KINEMATICS_PLUGIN_HPP
