//
// Created by Bailey Chessum on 14/10/2025.
//

#include <arm_kinematics/forward_kinematics_plugin.hpp>

#include <urdf/model.h>
#include <stdexcept>

#include <arm_kinematics/utilities.hpp>


namespace arm_kinematics {

bool ForwardKinematicsPlugin::initialize(ForwardKinematicsPlugin::KinematicsNodeInterfaces node_interfaces,
                                      std::string &robot_description, const std::vector<std::string> &joint_names) {
  robot_description_ = &robot_description;
  joint_names_ = joint_names;

  node_interfaces_ = node_interfaces;
  logger_ = node_interfaces.get_node_logging_interface()->get_logger().get_child("kinematics");

  // Set up URDF and KDL kinematics
  RCLCPP_INFO(get_logger(), "Parsing URDF and creating KDL Tree...");
  if (!urdf_model_.initString(robot_description)) {
    RCLCPP_ERROR(get_logger(), "Failed to init URDF model from robot_description string.");
    return false;
  }

  if (!kdl_parser::treeFromUrdfModel(urdf_model_, kdl_tree_)) {
    RCLCPP_ERROR(get_logger(), "Failed to convert URDF to KDL tree.");
    return false;
  }

  if (!kdl_tree_.getChain(get_kinematics_params().base_link_name, get_kinematics_params().ee_link_name,
                          kdl_chain_)) {
    RCLCPP_ERROR(get_logger(), "Failed to get KDL chain from \"%s\" to \"%s\".",
                 get_kinematics_params().base_link_name.c_str(), get_kinematics_params().ee_link_name.c_str());
    return false;
  }

  for (const auto & [name, joint] : urdf_model_.joints_) {
    if (joint->mimic) {
      mimic_joints.push_back({
                               .index = getJointIndex(joint->name),
                               .source_index = getJointIndex(joint->mimic->joint_name),
                               .multiplier = joint->mimic->multiplier,
                               .offset = joint->mimic->offset
                             });
    }
  }

  // Pre-allocate a JntArray to use with KDL in real-time contexts
  preallocated_jnts = KDL::JntArray()

  return on_initialize();
}

bool ForwardKinematicsPlugin::get_position_fk(const std::vector<double> &joint_angles,
                                           const std::string &link_name,
                                           Eigen::Isometry3d &solution_pose) const {
  // Default implementation using KDL



  return false;
}

bool ForwardKinematicsPlugin::get_position_fk(const std::vector<double> &joint_angles,
                                           Eigen::Isometry3d &solution_pose) const {
  // Default implementation just uses the general case implementation
  // But you could override this if you have an analytical solution

  return get_position_fk(joint_angles, get_kinematics_params().ee_link_name, solution_pose);
}


const std::vector<std::string> &ForwardKinematicsPlugin::get_joint_names() const noexcept {
  return joint_names_;
}

const std::string & ForwardKinematicsPlugin::get_robot_description() const {
  if (!robot_description_)
    throw std::logic_error("Tried to use a KinematicsPlugin before calling initialize() or after initialize() failed.");

  return *robot_description_;
}


const rclcpp::Logger &ForwardKinematicsPlugin::get_logger() const noexcept {
  return logger_;
}

const KinematicsParams &ForwardKinematicsPlugin::get_kinematics_params() const noexcept {
  return kinematics_params_;
}

const ForwardKinematicsPlugin::KinematicsNodeInterfaces & ForwardKinematicsPlugin::get_node_interfaces() const {
  if (!node_interfaces_.has_value())
    throw std::logic_error("Tried to use a KinematicsPlugin before calling initialize() or after initialize() failed.");

  return *node_interfaces_;
}


} // arm_kinematics