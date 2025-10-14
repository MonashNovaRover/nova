//
// Created by Bailey Chessum on 14/10/2025.
//

#include <arm_kinematics/kinematics_plugin_base.hpp>

#include <urdf/model.h>
#include <stdexcept>

namespace arm_kinematics {

bool KinematicsPluginBase::initialize(KinematicsPluginBase::KinematicsNodeInterfaces node_interfaces,
                                      std::string &robot_description, const std::vector<std::string> &joint_names) {
  robot_description_ = &robot_description;
  joint_names_ = joint_names;

  node_interfaces_ = node_interfaces;
  logger_ = node_interfaces.get_node_logging_interface()->get_logger().get_child("kinematics");

  return on_initialize();
}

bool KinematicsPluginBase::get_position_fk(const std::vector<double> &joint_angles,
                                           const std::string &link_name,
                                           Eigen::Isometry3d &solution_pose) const {
  // Default implementation using KDL
  return false;
}

bool KinematicsPluginBase::get_position_fk(const std::vector<double> &joint_angles,
                                           Eigen::Isometry3d &solution_pose) const {
  // Default implementation just uses the general case implementation
  // But you could override this if you have an analytical solution

  return get_position_fk(joint_angles, get_kinematics_params().ee_link_name, solution_pose);
}

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
  // Just call the above method, getting ik_seed_pose from forward kinematics
  Eigen::Isometry3d ik_seed_pose;
  const auto fk_result = get_position_fk(ik_seed_state, ik_seed_pose);

  return fk_result && get_velocity_ik(ik_twist, ik_seed_pose, ik_seed_state, solution, time_step);
}

const std::vector<std::string> &KinematicsPluginBase::get_joint_names() const noexcept {
  return joint_names_;
}

const std::string & KinematicsPluginBase::get_robot_description() const {
  if (!robot_description_)
    throw std::logic_error("Tried to use a KinematicsPlugin before calling initialize() or after initialize() failed.");

  return *robot_description_;
}


const rclcpp::Logger &KinematicsPluginBase::get_logger() const noexcept {
  return logger_;
}

const KinematicsParams &KinematicsPluginBase::get_kinematics_params() const noexcept {
  return kinematics_params_;
}

const KinematicsPluginBase::NodeInterfaces &KinematicsPluginBase::get_node_interfaces() const {
  if (!node_interfaces_.has_value())
    throw std::logic_error("Tried to use a KinematicsPlugin before calling initialize() or after initialize() failed.");

  return *node_interfaces_;
}


} // arm_kinematics