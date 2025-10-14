//
// Created by Bailey Chessum on 14/10/2025.
//

#include <arm_kinematics/kinematics_plugin_base.hpp>

namespace arm_kinematics {



bool KinematicsPluginBase::get_velocity_ik(const Eigen::Matrix<double, 6, 1> &ik_twist,
                                           const Eigen::Isometry3d &ik_seed_pose,
                                           const std::vector<double> &ik_seed_state,
                                           std::vector<double> &solution,
                                           double time_step) const
{
  return false;
}

bool KinematicsPluginBase::get_velocity_ik(const Eigen::Matrix<double, 6, 1> &ik_twist,
                                           const std::vector<double> &ik_seed_state,
                                           std::vector<double> &solution,
                                           double time_step) const
{
  get_position_fk()
}

const std::vector<std::string> &KinematicsPluginBase::get_joint_names() const noexcept {
  return joint_names_;
}

std::string & KinematicsPluginBase::get_robot_description() const noexcept {
  return *robot_description_;
}

bool KinematicsPluginBase::get_position_fk(const std::vector<double> &joint_angles, const std::string &link_name,
                                           Eigen::Isometry3d &solution_pose) const {
  return false;
}

const rclcpp::Logger &KinematicsPluginBase::get_logger() const noexcept {
  return logger_;
}

const std::string &KinematicsPluginBase::get_ee_link_name() const noexcept {
  return ee_link_name_;
}


} // arm_kinematics