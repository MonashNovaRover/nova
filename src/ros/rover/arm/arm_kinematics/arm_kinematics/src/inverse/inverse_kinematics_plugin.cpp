//
// Created by Bailey Chessum on 15/10/2025.
//

#include "arm_kinematics/inverse/inverse_kinematics_plugin.hpp"

#include <cassert>
#include <utility>
#include <variant>

#include "arm_kinematics/utilities/utilities.hpp"

namespace arm_kinematics {

std::string IKFailure::NoSolution::format() const
{
  return "InverseKinematicsPlugin: no solution found" +
         (detail.empty() ? std::string{} : ": " + detail);
}

std::string IKFailure::InvalidSeed::format() const
{
  return "InverseKinematicsPlugin: invalid seed state" +
         (detail.empty() ? std::string{} : ": " + detail);
}

std::string IKFailure::InvalidTarget::format() const
{
  return "InverseKinematicsPlugin: invalid target" +
         (detail.empty() ? std::string{} : ": " + detail);
}

std::string IKFailure::OutsideWorkspace::format() const
{
  return "InverseKinematicsPlugin: target lies outside the workspace" +
         (detail.empty() ? std::string{} : ": " + detail);
}

std::string IKFailure::Singular::format() const
{
  return "InverseKinematicsPlugin: singular configuration encountered" +
         (detail.empty() ? std::string{} : ": " + detail);
}

std::string IKFailure::BackendError::format() const
{
  return "InverseKinematicsPlugin: backend error" +
         (detail.empty() ? std::string{} : ": " + detail);
}

std::string IKFailure::format() const
{
  return std::visit([](const auto & error) { return error.format(); }, value);
}

bool InverseKinematicsPlugin::initialize(
  const KinematicsNodeInterfaces & node_interfaces,
  const RobotModel & robot_model,
  KinematicsParams::SharedPtr kinematics_params)
{
  if (!initialize_base(node_interfaces, robot_model, std::move(kinematics_params), "inverse_kinematics"))
    return false;

  return true;
}

IKResult InverseKinematicsPlugin::get_velocity_ik(const Twistd & ik_twist,
                                                  const Eigen::Isometry3d & ik_seed_pose,
                                                  const span<const double> ik_seed_state,
                                                  const span<double> solution_velocities,
                                                  const double time_step) const
{
  if (time_step == 0.0) {
    return tl::unexpected(IKFailure::BackendError{
      "get_velocity_ik() requires time_step to be non-zero"});
  }

  // Apply the ik_twist over time_step to get another pose
  const auto twist_applied_pose = apply_twist(ik_twist, time_step, ik_seed_pose);

  // solution_velocities will contain twist to avoid allocating a vector
  auto ik_result = get_position_ik(twist_applied_pose, ik_seed_state, solution_velocities);

  if (!ik_result) {
    return tl::unexpected(std::move(ik_result.error()));
  }

  assert(solution_velocities.size() == ik_seed_state.size());

  // Get the difference of the resulting joint angles for the time_step to get velocity
  for (size_t i = 0; i < solution_velocities.size(); ++i) {
    solution_velocities[i] = (solution_velocities[i] - ik_seed_state[i]) / time_step;
  }

  return {};
}

} // arm_kinematics
