//
// Created by nova on 12/28/25.
//

#ifndef SCIENCE_JOINT_HANDLE_HPP
#define SCIENCE_JOINT_HANDLE_HPP

#include <hardware_interface/loaned_state_interface.hpp>

#include "joint_state_handle.hpp"

namespace arm_kinematics {

struct JointHandle {

  explicit JointHandle(
    const std::string & joint_name,
    const std::string);

  /**
   * A collection of optional loaned state interface pointers for position, velocity, and effort. Each pointer may be
   * nullptr.
   */
  JointStateHandle state{};

};

} // arm_kinematics

#endif //SCIENCE_JOINT_HANDLE_HPP