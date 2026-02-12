//
// Created by nova on 12/28/25.
//

#include "joint_states.hpp"

namespace arm_kinematics {

JointStates::JointStates(const size_t initial_size) {
  velocities.resize(initial_size, 0);
  positions.resize(initial_size, 0);
  accelerations.resize(initial_size, 0);
  effort.resize(initial_size, 0);
}

} // arm_kinematics