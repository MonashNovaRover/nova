/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	  banksia_kinematics_plugin
AUTHORS:    Arbab Ahmed, Bailey Chessum
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include <memory>
#include <string>
#include <vector>

#include "banksia_kinematics_plugin/banksia_kinematics_plugin.hpp"
#include "rclcpp/logging.hpp"
#include "tf2/LinearMath/Scalar.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include <moveit/robot_model/robot_model.hpp>
#include <moveit/robot_state/robot_state.hpp>
#include <tf2_eigen/tf2_eigen.hpp>
#include <Eigen/Geometry>
#include <Eigen/src/Core/MatrixBase.h>

// backport this :(
// This function is Mozilla Public License v. 2.0
// it is not in eigen 3.4.1, only in 5.0.0+
namespace Eigen {
inline Vector3d canonicalEulerAngles(Matrix3d coeff,
    Index a0, Index a1, Index a2) {
  /* Implemented from Graphics Gems IV */

  Vector3d res;
#define Index int
#define Scalar double

  const Index odd = ((a0 + 1) % 3 == a1) ? 0 : 1;
  const Index i = a0;
  const Index j = (a0 + 1 + odd) % 3;
  const Index k = (a0 + 2 - odd) % 3;

  if (a0 == a2) {
    // Proper Euler angles (same first and last axis).
    // The i, j, k indices enable addressing the input matrix as the XYX archetype matrix (see Graphics Gems IV),
    // where e.g. coeff(k, i) means third column, first row in the XYX archetype matrix:
    //  c2      s2s1              s2c1
    //  s2s3   -c2s1s3 + c1c3    -c2c1s3 - s1c3
    // -s2c3    c2s1c3 + c1s3     c2c1c3 - s1s3

    // Note: s2 is always positive.
    Scalar s2 = hypot(coeff(j, i), coeff(k, i));
    if (odd) {
      res[0] = atan2(coeff(j, i), coeff(k, i));
      // s2 is always positive, so res[1] will be within the canonical [0, pi] range
      res[1] = atan2(s2, coeff(i, i));
    } else {
      // In the !odd case, signs of all three angles are flipped at the very end. To keep the solution within the
      // canonical range, we flip the solution and make res[1] always negative here (since s2 is always positive,
      // -atan2(s2, c2) will always be negative). The final flip at the end due to !odd will thus make res[1] positive
      // and canonical. NB: in the general case, there are two correct solutions, but only one is canonical. For proper
      // Euler angles, flipping from one solution to the other involves flipping the sign of the second angle res[1] and
      // adding/subtracting pi to the first and third angles. The addition/subtraction of pi to the first angle res[0]
      // is handled here by flipping the signs of arguments to atan2, while the calculation of the third angle does not
      // need special adjustment since it uses the adjusted res[0] as the input and produces a correct result.
      res[0] = atan2(-coeff(j, i), -coeff(k, i));
      res[1] = -atan2(s2, coeff(i, i));
    }

    // With a=(0,1,0), we have i=0; j=1; k=2, and after computing the first two angles,
    // we can compute their respective rotation, and apply its inverse to M. Since the result must
    // be a rotation around x, we have:
    //
    //  c2  s1.s2 c1.s2                   1  0   0
    //  0   c1    -s1       *    M    =   0  c3  s3
    //  -s2 s1.c2 c1.c2                   0 -s3  c3
    //
    //  Thus:  m11.c1 - m21.s1 = c3  &   m12.c1 - m22.s1 = s3

    Scalar s1 = numext::sin(res.coeff(0));
    Scalar c1 = numext::cos(res.coeff(0));
    res[2] = atan2(c1 * coeff(j, k) - s1 * coeff(k, k), c1 * coeff(j, j) - s1 * coeff(k, j));
  } else {
    // Tait-Bryan angles (all three axes are different; typically used for yaw-pitch-roll calculations).
    // The i, j, k indices enable addressing the input matrix as the XYZ archetype matrix (see Graphics Gems IV),
    // where e.g. coeff(k, i) means third column, first row in the XYZ archetype matrix:
    //  c2c3    s2s1c3 - c1s3     s2c1c3 + s1s3
    //  c2s3    s2s1s3 + c1c3     s2c1s3 - s1c3
    // -s2      c2s1              c2c1

    res[0] = atan2(coeff(j, k), coeff(k, k));

    Scalar c2 = hypot(coeff(i, i), coeff(i, j));
    // c2 is always positive, so the following atan2 will always return a result in the correct canonical middle angle
    // range [-pi/2, pi/2]
    res[1] = atan2(-coeff(i, k), c2);

    Scalar s1 = numext::sin(res.coeff(0));
    Scalar c1 = numext::cos(res.coeff(0));
    res[2] = atan2(s1 * coeff(k, i) - c1 * coeff(j, i), c1 * coeff(j, j) - s1 * coeff(k, j));
  }
  if (!odd) {
    res = -res;
  }

  return res;
}
}

namespace banksia_kinematics_plugin
{
  bool BanksiaKinematicsPlugin::initialize(const rclcpp::Node::SharedPtr &node,
                                           const moveit::core::RobotModel &robot_model,
                                           const std::string &group_name,
                                           const std::string &base_frame,
                                           const std::vector<std::string> &tip_frames,
                                           double search_discretization) {
    RCLCPP_INFO(node->get_logger(), "Initializing BanksiaKinematicsPlugin");

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
    // These are based on URDF not physical arm. Increase end effector length because
    // it seems to overshoot?
    link_lengths_ = {0.5052, 0.6193, 0.2225+0.021040};

    return true;
  }


  // TODO: remember to add something for the effector pose
  std::array<double, 6> BanksiaKinematicsPlugin::calculate_ik(tf2::Transform pose, std::array<double, 3> lengths) const {
    auto logger = node_.lock()->get_logger();
    auto origin = pose.getOrigin();
    auto x = origin.getX();
    auto y = origin.getY();
    auto z = origin.getZ();

    tf2::Matrix3x3 rotated_basis = pose.getBasis();

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


    RCLCPP_DEBUG(logger, "IK TARGET");
    RCLCPP_DEBUG(logger, "[%4.2f %4.2f %4.2f %4.2f;", t07r(0,0), t07r(0,1), t07r(0,2), t07r(0,3));
    RCLCPP_DEBUG(logger, " %4.2f %4.2f %4.2f %4.2f;", t07r(1,0), t07r(1,1), t07r(1,2), t07r(1,3));
    RCLCPP_DEBUG(logger, " %4.2f %4.2f %4.2f %4.2f;", t07r(2,0), t07r(2,1), t07r(2,2), t07r(2,3));
    RCLCPP_DEBUG(logger, " %4.2f %4.2f %4.2f %4.2f]", t07r(3,0), t07r(3,1), t07r(3,2), t07r(3,3));

    // need two dh transforms to get x of end effector frame pointing forwards
    Eigen::Matrix4d t67 = sub_dh(M_PI/2, 0, 0, M_PI/2) * sub_dh(M_PI/2, l3, 0, 0);
    Eigen::Matrix4d t0_wrist = t07r * t67.inverse();

    double wrist_x = t0_wrist(0, 3);
    double wrist_y = t0_wrist(1, 3);
    double wrist_z = t0_wrist(2, 3);

    double j1 = atan2(wrist_y, wrist_x);
    double l = sqrt(pow(wrist_x, 2) + pow(wrist_y, 2));
    double j3a = acos((pow(l, 2) + pow(wrist_z, 2) - pow(l1r, 2) - pow(l2r, 2)) / (2 * l1r * l2r));
    double j3b = -j3a; // expands to -acos((l^2+wrist_z^2-L1r^2-L2r^2)/(2*L1r*L2r)) as per keenan's notes
    double j3ao = -j3a - M_PI / 2;
    double j3bo = -j3b - M_PI / 2;

    double k1a = l1r + l2r * cos(j3a);
    double k2a = l2r * sin(j3a);
    double k1b = l1r + l2r * cos(j3b);
    double k2b = l2r * sin(j3b);

    double j2a = atan2(wrist_z, l) - atan2(k2a, k1a);
    double j2b = atan2(wrist_z, l) - atan2(k2b, k1b);
    double j2ao = -j2a;
    double j2bo = -j2b;

    Eigen::Matrix4d t01 = sub_dh(0, 0, 0, j1);
    Eigen::Matrix4d t12 = sub_dh(-M_PI / 2, 0, 0, j2bo);
    Eigen::Matrix4d t23 = sub_dh(0, l1r, 0, j3bo);
    Eigen::Matrix4d t02 = t01 * t12;
    Eigen::Matrix4d t03_wrist = t02 * t23;
    Eigen::Matrix3d r03_wrist = t03_wrist.topLeftCorner<3, 3>(); // R03_wrist = T03_wrist(1:3,1:3); in matlab
    Eigen::Matrix3d r07r = t07r.topLeftCorner<3, 3>(); // see above
    Eigen::Matrix3d r37r = r03_wrist.inverse() * r07r;

    
    // I don't know why we have to roll the rows of the matrix by one.
    Eigen::Vector3i indicies = {1,2,0};
    Eigen::Matrix3d r37r_shifted = r37r(indicies,Eigen::all);
 

    // when converting the required wrist rotation to intrinsic roll pitch roll euler angles
    // there are multiple solutions. 2 axies have 360 degrees of rotation, and one has 180 degrees
    // of rotation. We can choose if the 180 degree one is roll1 or pitch (but not roll2).
    // "canonical" euler angles have the 2nd axis (pitch in this case) going 0-180. eigen also has
    // a function for it to be the first axis (roll1).
//#define CANONICAL_ANGLES

#ifdef CANONICAL_ANGLES
    // once we have eigen 5.0.0+ this can be done properly.
    // will return in range [-pi:pi]x[0:pi]x[-pi:pi]
    Eigen::Vector3d rpr = Eigen::canonicalEulerAngles(r37r_shifted,0,1,0);
#else
    // returns in range [0:pi]x[-pi:pi]x[-pi:pi]
    // trying this instead so j4 moves less
    Eigen::Vector3d rpr = r37r_shifted.eulerAngles(0,1,0);
#endif



    double j4 = -rpr(0); // ???
    double j5 = rpr(1) - M_PI/2;
    double j6 = rpr(2);

#ifndef CANONICAL_ANGLES
    // move j4's range from [-pi:0] to [-pi/2:pi/2]
    // if we rotate j4 180 degrees because of this, j5 and j6 need inverting too
    if (j4 < -M_PI/2) {
      j4 = j4 + M_PI;
      j5 = -rpr(1)-M_PI/2;
      j6 = j6 + (j6 < 0 ? M_PI : - M_PI);
    }
#endif

    // Needs to be in the same order as when they get put in a joint group ???
    std::array<double, 6> new_joints = { j1, j2bo+M_PI/2, j4, j5, j6, j3bo + j2bo + M_PI/2 };
    RCLCPP_DEBUG(logger, "IK SOLUTION");
    RCLCPP_DEBUG(logger, "%4.2f %4.2f %4.2f %4.2f %4.2f %4.2f", j1, j2bo+M_PI/2, j3bo + j2bo + M_PI/2, j4, j5, j6);
    return new_joints;
  }

  bool BanksiaKinematicsPlugin::getPositionIK(const geometry_msgs::msg::Pose &ik_pose,
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

  bool BanksiaKinematicsPlugin::searchPositionIK(const geometry_msgs::msg::Pose &ik_pose,
                                                 const std::vector<double> &ik_seed_state, double timeout,
                                                 std::vector<double> &solution,
                                                 moveit_msgs::msg::MoveItErrorCodes &error_code,
                                                 const kinematics::KinematicsQueryOptions &options) const {
    // TODO: Actually include a timeout
    return getPositionIK(ik_pose, ik_seed_state, solution, error_code, options);
  }

  bool BanksiaKinematicsPlugin::searchPositionIK(const geometry_msgs::msg::Pose &ik_pose,
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

  bool BanksiaKinematicsPlugin::getPositionFK(const std::vector<std::string> &link_names,
                                              const std::vector<double> &joint_angles,
                                              std::vector<geometry_msgs::msg::Pose> &poses) const {
    if (node_.expired())
      return false;
    auto logger = node_.lock()->get_logger();

    RCLCPP_DEBUG(logger, "joint_angles:");
    for (auto& joint_angle : joint_angles) {
      RCLCPP_DEBUG(logger, "  - %f", joint_angle);
    }

    RCLCPP_DEBUG(logger, "active joint model names in joint group:");
    for (auto& joint_angle : robot_model_->getJointModelGroup(group_name_)->getActiveJointModelNames()) {
      RCLCPP_DEBUG(logger, "  - %s", joint_angle.c_str());
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

  const std::vector<std::string> &BanksiaKinematicsPlugin::getJointNames() const {
    return joint_names_;
  }

  const std::vector<std::string> &BanksiaKinematicsPlugin::getLinkNames() const {
    return link_names_;
  }

  bool BanksiaKinematicsPlugin::searchPositionIK(const std::vector<geometry_msgs::msg::Pose> &ik_poses,
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

  bool BanksiaKinematicsPlugin::searchPositionIK(const geometry_msgs::msg::Pose &ik_pose,
                                                 const std::vector<double> &ik_seed_state, double timeout,
                                                 const std::vector<double> &consistency_limits,
                                                 std::vector<double> &solution,
                                                 moveit_msgs::msg::MoveItErrorCodes &error_code,
                                                 const kinematics::KinematicsQueryOptions &options) const {
    // TODO: Implement
    return false;
  }

  bool BanksiaKinematicsPlugin::searchPositionIK(const geometry_msgs::msg::Pose &ik_pose,
                                                 const std::vector<double> &ik_seed_state, double timeout,
                                                 const std::vector<double> &consistency_limits,
                                                 std::vector<double> &solution,
                                                 const kinematics::KinematicsBase::IKCallbackFn &solution_callback,
                                                 moveit_msgs::msg::MoveItErrorCodes &error_code,
                                                 const kinematics::KinematicsQueryOptions &options) const {
    // TODO: Implement
    return false;
  }
} // namespace banksia_kinematics_plugin

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(banksia_kinematics_plugin::BanksiaKinematicsPlugin, kinematics::KinematicsBase);