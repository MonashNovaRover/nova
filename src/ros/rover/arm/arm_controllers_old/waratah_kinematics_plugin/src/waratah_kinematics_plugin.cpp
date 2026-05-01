/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	  waratah_kinematics_plugin
AUTHORS:    Arbab Ahmed, Bailey Chessum
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include <memory>
#include <string>
#include <vector>

#include "waratah_kinematics_plugin/waratah_kinematics_plugin.hpp"
#include "rclcpp/logging.hpp"
#include "tf2/LinearMath/Scalar.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include <moveit/robot_model/robot_model.h>
#include <moveit/robot_state/robot_state.h>
#include <tf2_eigen/tf2_eigen.hpp>

namespace
{
  /// The inverse of the global orientation the kinematics origin frame wants to rotate to for a roll,pitch,yaw of 0,0,0
  // To find this value, set the rotation quat to 0,0,0,1. Ensure both the arm kinematics origin and endeffector
  // kinematics frames have the z axis up and x axis facing drive forward when in the zero position. Run rviz, and enter
  // IK mode from the zero position. Enter the rotation of the endeffector_kinematics frame after this.
  const auto ENDEFFECTOR_BASIS_INVERSE = tf2::Matrix3x3(tf2::Quaternion(0.5, -0.5, 0.5, 0.5)).inverse();
} // namespace

namespace waratah_kinematics_plugin
{
  bool WaratahKinematicsPlugin::initialize(const rclcpp::Node::SharedPtr &node,
                                           const moveit::core::RobotModel &robot_model,
                                           const std::string &group_name,
                                           const std::string &base_frame,
                                           const std::vector<std::string> &tip_frames,
                                           double search_discretization) {
    RCLCPP_INFO(node->get_logger(), "Initializing WaratahKinematicsPlugin");

    setValues(robot_model.getName(), group_name, base_frame, tip_frames, search_discretization);

    node_ = std::weak_ptr<rclcpp::Node>(node);
    joint_names_ = robot_model.getJointModelGroup(group_name)->getActiveJointModelNames();
    link_names_ = robot_model.getJointModelGroup(group_name)->getLinkModelNames();

    // the robot model needs to outlive this class, and be cleaned up by the caller!
    robot_model_ = std::shared_ptr<const moveit::core::RobotModel>(&robot_model, [](const moveit::core::RobotModel*){/* no-op deleter */});

    if (!robot_model_) {
      RCLCPP_ERROR(node->get_logger(), "robot_model_ pointer is not initialized.");
    }

    // TODO: Calculate from the given RobotModel
    link_lengths_ = {0.5, 0.41799975417, 0.417};

    return true;
  }

  // See Keenan's IK notes
  // TODO: remember to add something for the effector pose
  std::array<double, 6> WaratahKinematicsPlugin::calculate_ik(tf2::Transform pose, std::array<double, 3> lengths) const {
    auto origin = pose.getOrigin();
    auto x = origin.getX();
    auto y = origin.getY();
    auto z = origin.getZ();

    tf2::Matrix3x3 rotated_basis = pose.getBasis() * ENDEFFECTOR_BASIS_INVERSE;

    double l1r = lengths[0];
    double l2r = lengths[1];
    double l3 = lengths[2];

    Eigen::Matrix3d rxyz {
      {rotated_basis[0][0], rotated_basis[0][1], rotated_basis[0][2]},
      {rotated_basis[1][0], rotated_basis[1][1], rotated_basis[1][2]},
      {rotated_basis[2][0], rotated_basis[2][1], rotated_basis[2][2]}
    };  // rxyz orientation matrix

    Eigen::Matrix4d t07r = Eigen::Matrix4d::Zero();
    t07r.topLeftCorner<3,3>() = rxyz;

    t07r(3, 3) = 1;
    t07r(0, 3) = x;
    t07r(1, 3) = y;
    t07r(2, 3) = z;

    Eigen::Matrix4d t67 { {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, l3}, {0, 0, 0, 1} };
    Eigen::Matrix4d t0_wrist = t07r * t67.inverse();

    double wrist_x = t0_wrist(0, 3);
    double wrist_y = t0_wrist(1, 3);
    double wrist_z = t0_wrist(2, 3);

    double j1 = atan2(wrist_y, wrist_x);
    double l = sqrt(pow(wrist_x, 2) + pow(wrist_y, 2));
    double j3a = acos((pow(l, 2) + pow(wrist_z, 2) - pow(l1r, 2) - pow(l2r, 2)) / (2 * l1r * l2r));
    double j3b = -j3a; // expands to -acos((l^2+wrist_z^2-L1r^2-L2r^2)/(2*L1r*L2r)) as per keenan's notes
    double j3ao = j3a + M_PI / 2;
    double j3bo = j3b + M_PI / 2;

    double k1a = l1r + l2r * cos(j3a);
    double k2a = l2r * sin(j3a);
    double k1b = l1r + l2r * cos(j3b);
    double k2b = l2r * sin(j3b);

    double j2a = atan2(wrist_z, l) - atan2(k2a, k1a);
    double j2b = atan2(wrist_z, l) - atan2(k2b, k1b);
    double j2ao = j2a - M_PI / 2;
    double j2bo = j2b - M_PI / 2;

    Eigen::Matrix4d t01 = sub_dh(0, 0, 0, j1);
    Eigen::Matrix4d t12 = sub_dh(M_PI / 2, 0, 0, j2bo + M_PI / 2);
    Eigen::Matrix4d t23 = sub_dh(0, l1r, 0, j3bo - M_PI / 2);
    Eigen::Matrix4d t02 = t01 * t12;
    Eigen::Matrix4d t03_wrist = t02 * t23;
    Eigen::Matrix3d r03_wrist = t03_wrist.topLeftCorner<3, 3>(); // R03_wrist = T03_wrist(1:3,1:3); in matlab
    Eigen::Matrix3d r07r = t07r.topLeftCorner<3, 3>(); // see above
    Eigen::Matrix3d r37r = r03_wrist.inverse() * r07r;

    double j4 = atan2(r37r(1, 2), r37r(0, 2));
    double j5 = atan2(-r37r(2, 2), r37r(0, 2) / cos(j4));
    double j6 = atan2(-r37r(2, 1) / cos(j5), r37r(2, 0) / cos(j5));

    // Needs to be in the same order as when they get put in a joint group
    std::array<double, 6> new_joints = { j1, j2bo, -j4, j5, j6, j3bo + j2bo };
    return new_joints;
  }

  bool WaratahKinematicsPlugin::getPositionIK(const geometry_msgs::msg::Pose &ik_pose,
                                              const std::vector<double> &ik_seed_state,
                                              std::vector<double> &solution,
                                              moveit_msgs::msg::MoveItErrorCodes &error_code,
                                              const kinematics::KinematicsQueryOptions &options) const {
    tf2::Transform tf_pose;
    tf2::fromMsg(ik_pose, tf_pose);

    auto joints = calculate_ik(tf_pose, link_lengths_);
    solution.assign(joints.begin(), joints.end());
    error_code.val = moveit_msgs::msg::MoveItErrorCodes::SUCCESS;
    return true;
  }

  bool WaratahKinematicsPlugin::searchPositionIK(const geometry_msgs::msg::Pose &ik_pose,
                                                 const std::vector<double> &ik_seed_state, double timeout,
                                                 std::vector<double> &solution,
                                                 moveit_msgs::msg::MoveItErrorCodes &error_code,
                                                 const kinematics::KinematicsQueryOptions &options) const {
    // TODO: Actually include a timeout
    return getPositionIK(ik_pose, ik_seed_state, solution, error_code, options);
  }

  bool WaratahKinematicsPlugin::searchPositionIK(const geometry_msgs::msg::Pose &ik_pose,
                                                 const std::vector<double> &ik_seed_state,
                                                 double timeout,
                                                 std::vector<double> &solution,
                                                 const kinematics::KinematicsBase::IKCallbackFn &solution_callback,
                                                 moveit_msgs::msg::MoveItErrorCodes &error_code,
                                                 const kinematics::KinematicsQueryOptions &options) const {
    auto solution_found = searchPositionIK(ik_pose, ik_seed_state, timeout, solution, error_code, options);
    solution_callback(ik_pose, solution, error_code);
    return solution_found;
  }

  bool WaratahKinematicsPlugin::getPositionFK(const std::vector<std::string> &link_names,
                                              const std::vector<double> &joint_angles,
                                              std::vector<geometry_msgs::msg::Pose> &poses) const {
    if (node_.expired())
      return false;
    auto logger = node_.lock()->get_logger();

    RCLCPP_DEBUG(logger, "joint_angles:");
    for (auto& joint_angle : joint_angles) {
      RCLCPP_INFO(logger, "  - %f", joint_angle);
    }

    RCLCPP_DEBUG(logger, "active joint model names in joint group:");
    for (auto& joint_angle : robot_model_->getJointModelGroup(group_name_)->getActiveJointModelNames()) {
      RCLCPP_INFO(logger, "  - %s", joint_angle.c_str());
    }

    // TODO: Actually solve FK
    poses.clear();

    if (!robot_model_)
    {
      RCLCPP_ERROR(logger, "Robot model is not initialized.");
      return false;
    }

    const moveit::core::JointModelGroup* joint_model_group =
      robot_model_->getJointModelGroup(group_name_);
    if (!joint_model_group)
    {
      RCLCPP_ERROR(logger, "Joint model group '%s' not found.", group_name_.c_str());
      return false;
    }

    moveit::core::RobotState robot_state(robot_model_);

    robot_state.setToDefaultValues();  // optional: ensure known base state
    robot_state.setJointGroupPositions(joint_model_group, joint_angles);
    robot_state.update();

    // Apply to mimic joints
    for (const auto* joint_model : robot_model_->getMimicJointModels()) {
      if (!joint_model)
        continue;

      const auto* source_joint = joint_model->getMimic();
      if (!source_joint)
        continue;

      const auto* position_ptr = robot_state.getJointPositions(source_joint);
      if (!position_ptr)
        continue;

      const auto position = joint_model->getMimicFactor() * (*position_ptr) + joint_model->getMimicOffset();
      robot_state.setVariablePosition(joint_model->getName(), position);
    }
    robot_state.update();

    /*
    RCLCPP_INFO(logger, "Full joint state (including mimic joints):");
    for (const auto* joint_model : robot_model_->getJointModels()) {
      if (!joint_model)
        continue;

      const std::string& name = joint_model->getName();
      auto positions_ptr = robot_state.getJointPositions(joint_model);

      auto position = (!positions_ptr) ? -9999 : (*positions_ptr) ;

      // Check if this is a mimic joint
      if (joint_model->getMimic()) {
        const auto* source_joint = joint_model->getMimic();


        RCLCPP_INFO(logger, "  - %s [mimics %s]: %f", name.c_str(), source_joint->getName().c_str(), position);
      }
      else {
        RCLCPP_INFO(logger, "  - %s: %f", name.c_str(), position);
      }
    }
    */

    auto base_transform_inverse = robot_state.getFrameTransform(base_frame_).inverse();

    poses.reserve(link_names.size());
    for (const auto& link_name : link_names)
    {
      const Eigen::Isometry3d& tf = base_transform_inverse * robot_state.getGlobalLinkTransform(link_name);
      geometry_msgs::msg::Pose pose = tf2::toMsg(tf);
      RCLCPP_DEBUG(logger, "%s pose:\n%s", link_name.c_str(), to_yaml(pose, false).c_str());
      poses.push_back(pose);
    }

    return true;
  }

  const std::vector<std::string> &WaratahKinematicsPlugin::getJointNames() const {
    return joint_names_;
  }

  const std::vector<std::string> &WaratahKinematicsPlugin::getLinkNames() const {
    return link_names_;
  }

  bool WaratahKinematicsPlugin::searchPositionIK(const std::vector<geometry_msgs::msg::Pose> &ik_poses,
                                                 const std::vector<double> &ik_seed_state, double timeout,
                                                 const std::vector<double> &consistency_limits,
                                                 std::vector<double> &solution,
                                                 const kinematics::KinematicsBase::IKCallbackFn &solution_callback,
                                                 const kinematics::KinematicsBase::IKCostFn &cost_function,
                                                 moveit_msgs::msg::MoveItErrorCodes &error_code,
                                                 const kinematics::KinematicsQueryOptions &options,
                                                 const moveit::core::RobotState *context_state) const {
    // TODO: Implement
    return false;
  }

  bool WaratahKinematicsPlugin::searchPositionIK(const geometry_msgs::msg::Pose &ik_pose,
                                                 const std::vector<double> &ik_seed_state, double timeout,
                                                 const std::vector<double> &consistency_limits,
                                                 std::vector<double> &solution,
                                                 moveit_msgs::msg::MoveItErrorCodes &error_code,
                                                 const kinematics::KinematicsQueryOptions &options) const {
    // TODO: Implement
    return false;
  }

  bool WaratahKinematicsPlugin::searchPositionIK(const geometry_msgs::msg::Pose &ik_pose,
                                                 const std::vector<double> &ik_seed_state, double timeout,
                                                 const std::vector<double> &consistency_limits,
                                                 std::vector<double> &solution,
                                                 const kinematics::KinematicsBase::IKCallbackFn &solution_callback,
                                                 moveit_msgs::msg::MoveItErrorCodes &error_code,
                                                 const kinematics::KinematicsQueryOptions &options) const {
    // TODO: Implement
    return false;
  }
} // namespace waratah_kinematics_plugin

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(waratah_kinematics_plugin::WaratahKinematicsPlugin, kinematics::KinematicsBase);