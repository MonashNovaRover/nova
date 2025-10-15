//
// Created by Bailey Chessum on 15/10/2025.
//

#include <arm_kinematics/inverse_kinematics_plugin.hpp>
#include <arm_kinematics/utilities.hpp>

namespace arm_kinematics {

bool InverseKinematicsPlugin::get_velocity_ik(const Eigen::Matrix<double, 6, 1> &ik_twist,
                                              const Eigen::Isometry3d &ik_seed_pose,
                                              const std::vector<double> &ik_seed_state,
                                              std::vector<double> &solution_velocities,
                                              double time_step) const
{
  assert(time_step != 0);

  // Apply the ik_twist over time_step to get another pose
  auto twist_applied_pose = apply_twist(ik_twist, time_step, ik_seed_pose);

  // solution_velocities will contain twist to avoid allocating a vector
  auto ik_result = get_position_ik(twist_applied_pose, ik_seed_state, solution_velocities);

  // TODO: Should we attempt to make this real time safe, and avoid early exits?
  if (!ik_result)
    return false;

  assert(solution_velocities.size() == ik_seed_state.size());

  // Get the difference of the resulting joint angles for the time_step to get velocity
  for (size_t i = 0; i < solution_velocities.size(); ++i) {
    solution_velocities[i] = (solution_velocities[i] - ik_seed_state[i]) / time_step;
  }

  return true;
}

bool InverseKinematicsPlugin::get_velocity_ik(const Eigen::Matrix<double, 6, 1> &ik_twist,
                                              const ForwardKinematicsPlugin::SharedPtr fk,
                                              const std::vector<double> &ik_seed_state,
                                              std::vector<double> &solution_velocities,
                                              double time_step) const
{
  if (!fk) {
    RCLCPP_ERROR(get_logger(), "Given ForwardKinematicsPlugin::SharedPtr is not valid!");
    return false;
  }

  // Just call the above method, getting ik_seed_pose from forward kinematics
  Eigen::Isometry3d ik_seed_pose;
  const auto fk_result = fk->get_position_fk(ik_seed_state, ik_seed_pose);

  return fk_result && get_velocity_ik(ik_twist, ik_seed_pose, ik_seed_state, solution_velocities, time_step);
}
} // arm_kinematics