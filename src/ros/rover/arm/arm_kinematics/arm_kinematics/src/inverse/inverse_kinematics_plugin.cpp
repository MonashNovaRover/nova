//
// Created by Bailey Chessum on 15/10/2025.
//

#include "arm_kinematics/inverse/inverse_kinematics_plugin.hpp"
#include "arm_kinematics/utilities/utilities.hpp"

namespace arm_kinematics {

bool InverseKinematicsPlugin::initialize(
  const KinematicsNodeInterfaces & node_interfaces,
  const RobotModel & robot_model,
  KinematicsParams::SharedPtr kinematics_params)
{
  if (!initialize_base(node_interfaces, robot_model, std::move(kinematics_params), "inverse_kinematics"))
    return false;

  return true;
}

bool InverseKinematicsPlugin::get_velocity_ik(const Twistf & ik_twist,
                                              const Eigen::Isometry3f &ik_seed_pose,
                                              const std::vector<double> &ik_seed_state,
                                              std::vector<double> &solution_velocities,
                                              const double time_step) const
{
  assert(time_step != 0);

  // Apply the ik_twist over time_step to get another pose
  const auto twist_applied_pose = apply_twist(ik_twist, time_step, ik_seed_pose);

  // solution_velocities will contain twist to avoid allocating a vector
  const auto ik_result = get_position_ik(twist_applied_pose, ik_seed_state, solution_velocities);

  if (!ik_result)
    return false;

  assert(solution_velocities.size() == ik_seed_state.size());

  // Get the difference of the resulting joint angles for the time_step to get velocity
  for (size_t i = 0; i < solution_velocities.size(); ++i) {
    solution_velocities[i] = (solution_velocities[i] - ik_seed_state[i]) / time_step;
  }

  return true;
}

} // arm_kinematics