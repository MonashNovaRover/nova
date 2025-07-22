/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

A MoveIt2 kinematics::KinematicsBase plugin for
  Waratah's robotic arm payload, based on the
  DH tables and inverse kinematics solutions by
  Keenan.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:  waratah_kinematics_plugin
AUTHORS:  Arbab Ahmed, Bailey Chessum
CREATION:	19/04/2025
EDITED:		20/04/2025
EDITED BY:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Make IK fail when the target pose is out of
   reach of the arm.
 - Automatically calculate linkage lengths based
   on the given robot model
 - Make use of seed_state to choose the closest
   solution for IK, rather than only elbow up.
 - Implement remaining functions for MoveIt2
 - Try reducing the number of type conversions
   needed
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#ifndef WARATAH_KINEMATICS_PLUGIN__WARATAH_KINEMATICS_PLUGIN_HPP_
#define WARATAH_KINEMATICS_PLUGIN__WARATAH_KINEMATICS_PLUGIN_HPP_

#include <chrono>
#include <cmath>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "visibility_control.h"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp/node.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "tf2/LinearMath/Scalar.h"
#include <Eigen/Dense>
#include <tf2/LinearMath/Transform.h>
#include <moveit/kinematics_base/kinematics_base.h>

namespace waratah_kinematics_plugin
{
class WaratahKinematicsPlugin : public kinematics::KinematicsBase
{
  bool initialize(
    const rclcpp::Node::SharedPtr &node,
    const moveit::core::RobotModel &robot_model,
    const std::string &group_name,
    const std::string &base_frame,
    const std::vector<std::string> &tip_frames,
    double search_discretization) override;

  bool getPositionIK(
    const geometry_msgs::msg::Pose &ik_pose,
    const std::vector<double> &ik_seed_state,
    std::vector<double> &solution,
    moveit_msgs::msg::MoveItErrorCodes &error_code,
    const kinematics::KinematicsQueryOptions &options = kinematics::KinematicsQueryOptions()) const override;

  bool searchPositionIK(
    const geometry_msgs::msg::Pose &ik_pose,
    const std::vector<double> &ik_seed_state,
    double timeout,
    std::vector<double> &solution,
    moveit_msgs::msg::MoveItErrorCodes &error_code,
    const kinematics::KinematicsQueryOptions &options = kinematics::KinematicsQueryOptions()) const override;

  bool searchPositionIK(
    const std::vector<geometry_msgs::msg::Pose> &ik_poses,
    const std::vector<double> &ik_seed_state,
    double timeout,
    const std::vector<double> &consistency_limits,
    std::vector<double> &solution,
    const kinematics::KinematicsBase::IKCallbackFn &solution_callback,
    const kinematics::KinematicsBase::IKCostFn &cost_function,
    moveit_msgs::msg::MoveItErrorCodes &error_code,
    const kinematics::KinematicsQueryOptions &options = kinematics::KinematicsQueryOptions(),
    const moveit::core::RobotState *context_state = nullptr) const override;

  bool searchPositionIK(
    const geometry_msgs::msg::Pose &ik_pose,
    const std::vector<double> &ik_seed_state,
    double timeout,
    std::vector<double> &solution,
    const IKCallbackFn &solution_callback,
    moveit_msgs::msg::MoveItErrorCodes &error_code,
    const kinematics::KinematicsQueryOptions &options = kinematics::KinematicsQueryOptions()) const override;

  bool searchPositionIK(
    const geometry_msgs::msg::Pose& ik_pose,
    const std::vector<double>& ik_seed_state,
    double timeout,
    const std::vector<double>& consistency_limits,
    std::vector<double>& solution,
    moveit_msgs::msg::MoveItErrorCodes& error_code,
    const kinematics::KinematicsQueryOptions& options = kinematics::KinematicsQueryOptions()) const override;

  bool searchPositionIK(
    const geometry_msgs::msg::Pose& ik_pose,
    const std::vector<double>& ik_seed_state,
    double timeout,
    const std::vector<double>& consistency_limits,
    std::vector<double>& solution,
    const IKCallbackFn& solution_callback,
    moveit_msgs::msg::MoveItErrorCodes& error_code,
    const kinematics::KinematicsQueryOptions& options = kinematics::KinematicsQueryOptions()) const override;

  bool getPositionFK(
    const std::vector<std::string> &link_names,
    const std::vector<double> &joint_angles,
    std::vector<geometry_msgs::msg::Pose> &poses) const override;

  const std::vector<std::string> &getJointNames() const override;

  const std::vector<std::string> &getLinkNames() const override;

private:
  /// @brief Calculates the IK given a frame, pose and set of lengths.
  /// @brief See Keenan's IK notes.
  /// @param frame is ???
  /// @param pose is a struct combining a position (x,y,z) and a quaternion (x,y,z,w).
  /// @param Lengths are [fill this].
  /// @returns an array of joint angles in joint space for each of J1 through J6, through joints.
  std::array<double, 6> calculate_ik(tf2::Transform pose, std::array<double, 3> lengths) const;

  // substitutes values into our DH table.
  // see: https://www.notion.so/Inverse-Kinematics-ddfe35179c1f4959850bd28b2195be8a
  // equivalent line: DHs = [cos(the) -sin(the) 0 a; sin(the)*cos(alp) cos(the)*cos(alp) -sin(alp) -sin(alp)*d; sin(the)*sin(alp) cos(the)*sin(alp) cos(alp) cos(alp)*d; 0 0 0 1];
  Eigen::Matrix4d sub_dh(double alp, double a, double d, double the) const
  {
    return Eigen::Matrix4d {
      { cos(the), 			-sin(the), 			0, 			a },
      { sin(the)*cos(alp),	cos(the)*cos(alp), 	-sin(alp), 	-sin(alp)*d },
      { sin(the)*sin(alp),	cos(the)*sin(alp),	cos(alp),	cos(alp)*d },
      { 0,					0,					0,			1 }
    };
  }

  std::vector<std::string> joint_names_;
  std::vector<std::string> link_names_;
  std::array<double, 3> link_lengths_;  // L1, L2, L3

  rclcpp::Node::WeakPtr node_;
};
} // namespace waratah_kinematics_plugin
#endif // WARATAH_KINEMATICS_PLUGIN__WARATAH_KINEMATICS_PLUGIN_HPP_
