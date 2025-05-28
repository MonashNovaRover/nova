/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	  waratah_kinematics_plugin
AUTHORS:    Arbab Ahmed, Bailey Chessum, Orlando Chamberlain
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
    // l1r l2r a1r a2r a3r
    link_lengths_ = {0.485, 0.5036, 0.119, 0.102, 0.31526};

    return true;
  }

  // See Keenan's IK notes
  // TODO: remember to add something for the effector pose
  std::array<double, 6> WaratahKinematicsPlugin::calculate_ik(tf2::Transform pose, std::array<double, LINK_LENGTH_COUNT> lengths) const {
    auto origin = pose.getOrigin();
    auto x = origin.getX();
    auto y = origin.getY();
    auto z = origin.getZ();

    auto logger = node_.lock()->get_logger();

    tf2::Matrix3x3 rotated_basis = pose.getBasis() * ENDEFFECTOR_BASIS_INVERSE;

    double l1r = lengths[0];
    double l2r = lengths[1];
    double a1r = lengths[2];
    double a2r = lengths[3];
    double a3r = lengths[4];

    Eigen::Matrix3d rxyz {
      {rotated_basis[0][0], rotated_basis[0][1], rotated_basis[0][2]},
      {rotated_basis[1][0], rotated_basis[1][1], rotated_basis[1][2]},
      {rotated_basis[2][0], rotated_basis[2][1], rotated_basis[2][2]}
    };  // rxyz orientation matrix

    // set the input requested transform from 0 to 7
    Eigen::Matrix4d t07i = Eigen::Matrix4d::Zero();
    t07i.topLeftCorner<3,3>() = rxyz;

    t07i(3, 3) = 1;
    t07i(0, 3) = x;
    t07i(1, 3) = y;
    t07i(2, 3) = z;

    RCLCPP_INFO(logger, "target:");
    RCLCPP_INFO(logger, "[%+5.3f, %+5.3f, %+5.3f, %+5.3f; ...", t07i(0,0), t07i(0,1), t07i(0,2), t07i(0,3));
    RCLCPP_INFO(logger, " %+5.3f, %+5.3f, %+5.3f, %+5.3f; ...", t07i(1,0), t07i(1,1), t07i(1,2), t07i(1,3));
    RCLCPP_INFO(logger, " %+5.3f, %+5.3f, %+5.3f, %+5.3f; ...", t07i(2,0), t07i(2,1), t07i(2,2), t07i(2,3));
    RCLCPP_INFO(logger, " %+5.3f, %+5.3f, %+5.3f, %+5.3f]",     t07i(3,0), t07i(3,1), t07i(3,2), t07i(3,3));

#define T01(j1) sub_dh(0,       0,   0,   j1        )
#define T12(j2) sub_dh(M_PI/2,  0,   0,   j2        )
#define T23(j3) sub_dh(0,       l1r, 0,   j3        )
#define T34(j4) sub_dh(0,       l2r, a1r, j4        )
#define T45(j5) sub_dh(-M_PI/2, 0,   a2r, j5+M_PI/2 )
#define T56(j6) sub_dh(M_PI/2,  0,   0,   j6        )
#define T67()   sub_dh(0,       0,   a3r, 0         )

    Eigen::Matrix4d t0_wrist = t07i * T67().inverse();

    double wrist_x = t0_wrist(0, 3);
    double wrist_y = t0_wrist(1, 3);
    //double wrist_z = t0_wrist(2, 3); // unused

    // J1 Solution
    double beta = atan2(wrist_y, wrist_x);
    double lp = sqrt(pow(wrist_x, 2) + pow(wrist_y, 2) - pow(a1r,2));
    double alpha = atan2(a1r, lp);
    double j1 = alpha+beta;

    // Remove J1 now that it is solved
    Eigen::Matrix4d t01c = T01(j1);
    Eigen::Matrix4d t17c = t01c.inverse() * t07i;

    // J56 Solution
    double j5 = atan2((t17c(1,3)+a1r)/a3r,sqrt(pow(t17c(1,0),2)+pow(t17c(1,1),2)));
    double j6 = atan2(-t17c(1,1),t17c(1,0));

    // Remove J56 now that they are solved
    Eigen::Matrix4d t47c = T45(j5)*T56(j6)*T67();
    Eigen::Matrix4d t14c = t01c.inverse() * t07i * t47c.inverse();

    // J3 Solution
    double b3 = (pow(t14c(0,3),2)+pow(t14c(2,3),2) - pow(l2r,2) - pow(l1r,2))  / (2*l1r*l2r); 
    double j3a = -atan2(sqrt(1-pow(b3,2)), b3);
    double j3b =  atan2(sqrt(1-pow(b3,2)), b3);

    // J2 Solution
    double j2a = atan2(t14c(2,3),t14c(0,3))-atan2(l2r*sin(j3a),l1r+l2r*cos(j3a));
    double j2b = atan2(t14c(2,3),t14c(0,3))-atan2(l2r*sin(j3b),l1r+l2r*cos(j3b));

    // J4 solution
    Eigen::Matrix4d t03ca = T01(j1)*T12(j2a)*T23(j3a);
    Eigen::Matrix4d t34ca = t03ca.inverse() * t07i * t47c.inverse();
    double j4a = atan2(t34ca(1,0), t34ca(0,0));
    
    Eigen::Matrix4d t03cb = T01(j1)*T12(j2b)*T23(j3b);
    Eigen::Matrix4d t34cb = t03cb.inverse() * t07i * t47c.inverse();
    double j4b = atan2(t34cb(1,0), t34cb(0,0));

    std::array<double, 6> new_joints_a = { j1, j2a - M_PI_2, j3a, j4a, j5, j6};
    std::array<double, 6> new_joints_b = { j1, j2b - M_PI_2, j3b, j4b, j5, j6};
    // TODO: pick which solution to use smartly
    
#define DEG(x) x*360/(2*M_PI)
    RCLCPP_INFO(logger, "solution: [%+07.2f %+07.2f %+07.2f  %+07.2f %+07.2f %+07.2f]", DEG(j1), DEG(j2a), DEG(j3a), DEG(j4a), DEG(j5), DEG(j6));

    return new_joints_a;
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