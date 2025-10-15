//
// Created by Bailey Chessum on 14/10/2025.
//

#include <arm_kinematics/forward_kinematics_plugin.hpp>

#include <urdf/model.h>
#include <stdexcept>

#include <arm_kinematics/utilities.hpp>
#include "arm_kinematics/joint_map_builder.hpp"
#include <kdl/chainfksolverpos_recursive.hpp>


namespace arm_kinematics {

bool ForwardKinematicsPlugin::initialize(ForwardKinematicsPlugin::KinematicsNodeInterfaces node_interfaces,
                                         std::string &robot_description, const std::vector<std::string> &joint_names,
                                         KinematicsParams kinematics_params) {
  if (!initialize_base(node_interfaces, robot_description, joint_names, std::move(kinematics_params),
                  "forward_kinematics"))
    return false;

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

  // Pre-allocate a JntArray to use with KDL in real-time contexts
  preallocated_jnts_ = KDL::JntArray(kdl_chain_.getNrOfJoints());

  joint_map_builder_ = JointMapBuilder()
    .with_urdf(urdf_model_)
    .with_transmissions(robot_description);
  chain_joint_map_ = joint_map_builder_.build(joint_names, kdl_chain_);

  return on_initialize();
}

bool ForwardKinematicsPlugin::get_position_fk(const std::vector<double> &joint_angles,
                                              const JointMap & joint_map,
                                              const KDL::Chain & kdl_chain,
                                              KDL::JntArray & kdl_chain_jnts,
                                              Eigen::Isometry3d & solution_pose) const {
  // Default implementation using KDL. I am assuming construction here is real-time safe with no heap allocations
  KDL::ChainFkSolverPos_recursive fk_solver(kdl_chain_);
  KDL::Frame out = KDL::Frame::Identity();  //< Should all be stack allocated!

  joint_map.copy_values_to_jnts(joint_angles, kdl_chain_jnts);
  bool result = fk_solver.JntToCart(kdl_chain_jnts, out) >= 0;

  kdl_to_eigen(out, solution_pose);
  return result;
}

bool ForwardKinematicsPlugin::get_position_fk(const std::vector<double> &joint_angles,
                                              Eigen::Isometry3d &solution_pose) {
  // Default implementation just uses the general case implementation
  // But you could override this if you have an analytical solution
  return get_position_fk(joint_angles, chain_joint_map_, kdl_chain_, preallocated_jnts_, solution_pose);
}

const KDL::Tree & ForwardKinematicsPlugin::get_kdl_tree() const noexcept {
  return kdl_tree_;
}

const JointMap & ForwardKinematicsPlugin::get_joint_map_builder() const noexcept {
  return joint_map_builder_;
}

} // arm_kinematics