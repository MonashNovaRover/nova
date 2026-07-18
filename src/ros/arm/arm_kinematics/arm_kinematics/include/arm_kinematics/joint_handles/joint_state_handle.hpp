//
// Created by nova on 12/28/25.
//

#ifndef SCIENCE_JOINT_STATE_HANDLE_HPP
#define SCIENCE_JOINT_STATE_HANDLE_HPP

#include <string>
#include <hardware_interface/loaned_state_interface.hpp>

namespace arm_kinematics {

/**
 * A collection of optional loaned state interface pointers for position, velocity, and effort. Each pointer may be
 * nullptr.
 */
struct JointStateHandle {
  explicit JointStateHandle() = default;
  explicit JointStateHandle(
    const std::string & joint_name,
    const std::vector<hardware_interface::LoanedStateInterface> & state_interfaces);

  const hardware_interface::LoanedStateInterface * position     = nullptr;
  const hardware_interface::LoanedStateInterface * velocity     = nullptr;
  const hardware_interface::LoanedStateInterface * acceleration = nullptr;
  const hardware_interface::LoanedStateInterface * effort       = nullptr;
};

} // arm_kinematics

#endif //SCIENCE_JOINT_STATE_HANDLE_HPP