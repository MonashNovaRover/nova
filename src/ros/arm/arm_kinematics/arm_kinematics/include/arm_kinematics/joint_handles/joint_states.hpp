//
// Created by nova on 12/28/25.
//

#ifndef SCIENCE_JOINT_STATES_HPP
#define SCIENCE_JOINT_STATES_HPP

#include <trajectory_msgs/msg/joint_trajectory_point.hpp>

namespace arm_kinematics {

class JointStates : public trajectory_msgs::msg::JointTrajectoryPoint {
  explicit JointStates(size_t initial_size);
};

} // arm_kinematics

#endif //SCIENCE_JOINT_STATES_HPP