//
// Created by Bailey Chessum on 14/10/2025.
//

#include <arm_kinematics/kinematics_plugins/forward_kinematics_plugin.hpp>

#include <urdf/model.h>
#include <stdexcept>

#include <arm_kinematics/utilities.hpp>
#include "arm_kinematics/joint_map/joint_map_builder.hpp"
#include <kdl/chainfksolverpos_recursive.hpp>


namespace arm_kinematics {

bool ForwardKinematicsPlugin::initialize(ForwardKinematicsPlugin::KinematicsNodeInterfaces node_interfaces,
                                         std::string &robot_description, const std::vector<std::string> &joint_names,
                                         KinematicsParams kinematics_params) {
  if (!initialize_base(node_interfaces, robot_description, joint_names, std::move(kinematics_params),
                  "forward_kinematics"))
    return false;

  // Set up URDF
  RCLCPP_INFO(get_logger(), "Parsing URDF and creating KDL Tree...");
  if (!urdf_model_.initString(robot_description)) {
    RCLCPP_ERROR(get_logger(), "Failed to init URDF model from robot_description string.");
    return false;
  }

  joint_map_builder_ = JointMapBuilder()
    .with_urdf(urdf_model_)
    .with_transmissions(robot_description);

  return on_initialize();
}

// bool ForwardKinematicsPlugin::get_position_fk(const std::vector<double> &joint_angles,
//                                               const JointMap & joint_map,
//                                               const KDL::Chain & kdl_chain,
//                                               KDL::JntArray & kdl_chain_jnts,
//                                               Eigen::Isometry3d & solution_pose) const {
//   // Default implementation using KDL. I am assuming construction here is real-time safe with no heap allocations
//   KDL::ChainFkSolverPos_recursive fk_solver(kdl_chain_);
//   KDL::Frame out = KDL::Frame::Identity();  //< Should all be stack allocated!
//
//   joint_map.map(joint_angles, kdl_chain_jnts);
//   bool result = fk_solver.JntToCart(kdl_chain_jnts, out) >= 0;
//
//   kdl_to_eigen(out, solution_pose);
//   return result;
// }
//
// bool ForwardKinematicsPlugin::get_position_fk(const std::vector<double> &joint_angles,
//                                               Eigen::Isometry3d &solution_pose) {
//   // Default implementation just uses the general case implementation
//   // But you could override this if you have an analytical solution
//   return get_position_fk(joint_angles, chain_joint_map_, kdl_chain_, preallocated_jnts_, solution_pose);
// }

const urdf::Model & ForwardKinematicsPlugin::get_urdf_model() const noexcept {
  return urdf_model_;
}

const JointMapBuilder & ForwardKinematicsPlugin::get_joint_map_builder() const noexcept {
  return joint_map_builder_;
}

} // arm_kinematics