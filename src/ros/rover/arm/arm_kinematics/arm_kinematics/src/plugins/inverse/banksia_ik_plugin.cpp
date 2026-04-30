//
// Created by Bailey Chessum on 15/12/2025.
//

#include "arm_kinematics/inverse/inverse_kinematics_plugin.hpp"

#include <array>
#include <cassert>
#include <cmath>

#include "arm_kinematics/common/robot_model.hpp"
#include "arm_kinematics/forward/utilities/analysis_tree.hpp"

namespace arm_kinematics {

namespace {

struct AxisLine
{
  Eigen::Vector3d point = Eigen::Vector3d::Zero();
  Eigen::Vector3d direction = Eigen::Vector3d::UnitZ();
};

constexpr size_t kExpectedBanksiaJointCount = 6;
constexpr size_t kShoulderJointIndex = 1;
constexpr size_t kElbowActuatedJointIndex = 2;
constexpr size_t kWristRollJointIndex = 3;
constexpr size_t kWristPitchJointIndex = 4;
constexpr size_t kWristYawJointIndex = 5;
constexpr char kBanksiaElbowPivotJointName[] = "l1_l2_pivot";

[[nodiscard]] AxisLine make_axis_line(
  const AnalysisTree & analysis_tree,
  const Eigen::Isometry3d & root_T_base,
  const std::string & joint_name)
{
  const auto joint = analysis_tree.query_joint(joint_name);
  const Eigen::Isometry3d base_T_joint = root_T_base.inverse() * joint.root_T_joint;

  return AxisLine{
    base_T_joint.translation(),
    (base_T_joint.rotation() * joint.axis_in_joint).normalized(),
  };
}

[[nodiscard]] Eigen::Vector3d closest_point_on_line(
  const AxisLine & line,
  const Eigen::Vector3d & point)
{
  return line.point + line.direction.dot(point - line.point) * line.direction;
}

[[nodiscard]] double distance_to_line(
  const AxisLine & line,
  const Eigen::Vector3d & point)
{
  return (point - closest_point_on_line(line, point)).norm();
}

[[nodiscard]] double distance_between_lines(
  const AxisLine & a,
  const AxisLine & b)
{
  const Eigen::Vector3d normal = a.direction.cross(b.direction);
  const double normal_norm = normal.norm();
  if (normal_norm < 1e-9) {
    return (a.direction.cross(b.point - a.point)).norm();
  }
  return std::abs(normal.normalized().dot(b.point - a.point));
}

[[nodiscard]] Eigen::Vector3d closest_midpoint_between_lines(
  const AxisLine & a,
  const AxisLine & b)
{
  const Eigen::Vector3d delta = b.point - a.point;
  const double aa = a.direction.dot(a.direction);
  const double bb = b.direction.dot(b.direction);
  const double ab = a.direction.dot(b.direction);
  const double denom = aa * bb - ab * ab;

  if (std::abs(denom) < 1e-9) {
    const Eigen::Vector3d on_a = closest_point_on_line(a, b.point);
    return 0.5 * (on_a + b.point);
  }

  const double ad = a.direction.dot(delta);
  const double bd = b.direction.dot(delta);
  const double ta = (ad * bb - bd * ab) / denom;
  const double tb = (ad * ab - bd * aa) / denom;
  const Eigen::Vector3d on_a = a.point + ta * a.direction;
  const Eigen::Vector3d on_b = b.point + tb * b.direction;
  return 0.5 * (on_a + on_b);
}

}  // namespace

/**
 * IK plugin for Taipan with the original (bad) carbon fibre wrist
 */
class BanksiaIKPlugin : public InverseKinematicsPlugin {
public:

  bool on_initialize() override {
    const auto & params = get_kinematics_params();
    const auto & analysis_tree = get_robot_model().get_analysis_tree();
    const auto & joint_order = analysis_tree.joint_name_order();
    const auto & frame_order = analysis_tree.frame_name_order();

    if (params.joint_names.size() < kExpectedBanksiaJointCount) {
      RCLCPP_ERROR(
        get_logger(),
        "Banksia IK expects at least %zu configured joint_names, got %zu.",
        kExpectedBanksiaJointCount,
        params.joint_names.size());
      return false;
    }

    const std::string & shoulder_joint_name = params.joint_names[kShoulderJointIndex];
    const std::string & elbow_actuated_joint_name = params.joint_names[kElbowActuatedJointIndex];
    const std::string & wrist_roll_joint_name = params.joint_names[kWristRollJointIndex];
    const std::string & wrist_pitch_joint_name = params.joint_names[kWristPitchJointIndex];
    const std::string & wrist_yaw_joint_name = params.joint_names[kWristYawJointIndex];

    if (!frame_order.contains_key(params.base_link_name)) {
      RCLCPP_ERROR(
        get_logger(),
        "Banksia IK could not find base_link_name '%s' in the AnalysisTree.",
        params.base_link_name.c_str());
      return false;
    }
    if (!frame_order.contains_key(params.ee_link_name)) {
      RCLCPP_ERROR(
        get_logger(),
        "Banksia IK could not find ee_link_name '%s' in the AnalysisTree.",
        params.ee_link_name.c_str());
      return false;
    }

    if (!joint_order.contains_key(shoulder_joint_name) ||
      !joint_order.contains_key(wrist_roll_joint_name) ||
      !joint_order.contains_key(wrist_pitch_joint_name) ||
      !joint_order.contains_key(wrist_yaw_joint_name))
    {
      RCLCPP_ERROR(
        get_logger(),
        "Banksia IK could not find the configured active chain joints in the AnalysisTree.");
      return false;
    }

    if (!joint_order.contains_key(kBanksiaElbowPivotJointName)) {
      RCLCPP_ERROR(
        get_logger(),
        "Banksia IK could not find the physical elbow pivot joint '%s' for configured elbow joint '%s'.",
        kBanksiaElbowPivotJointName,
        elbow_actuated_joint_name.c_str());
      return false;
    }

    const Eigen::Isometry3d root_T_base = analysis_tree.query_frame(params.base_link_name);

    const AxisLine shoulder_axis = make_axis_line(analysis_tree, root_T_base, shoulder_joint_name);
    const AxisLine elbow_axis = make_axis_line(analysis_tree, root_T_base, kBanksiaElbowPivotJointName);
    const AxisLine wrist_roll_axis = make_axis_line(analysis_tree, root_T_base, wrist_roll_joint_name);
    const AxisLine wrist_pitch_axis = make_axis_line(analysis_tree, root_T_base, wrist_pitch_joint_name);
    const AxisLine wrist_yaw_axis = make_axis_line(analysis_tree, root_T_base, wrist_yaw_joint_name);

    const Eigen::Vector3d wrist_center =
      (closest_midpoint_between_lines(wrist_roll_axis, wrist_pitch_axis) +
      closest_midpoint_between_lines(wrist_pitch_axis, wrist_yaw_axis) +
      closest_midpoint_between_lines(wrist_roll_axis, wrist_yaw_axis)) / 3.0;

    const Eigen::Vector3d ee_target =
      analysis_tree.query_transform_between_frames(params.base_link_name, params.ee_link_name)
      .translation();

    link_lengths_[0] = distance_between_lines(shoulder_axis, elbow_axis) +
      distance_to_line(shoulder_axis, Eigen::Vector3d::Zero());

    link_lengths_[1] = distance_to_line(elbow_axis, wrist_center);
    link_lengths_[2] = (ee_target - wrist_center).norm();

    if (!std::isfinite(link_lengths_[0]) || !std::isfinite(link_lengths_[1]) ||
      !std::isfinite(link_lengths_[2]) || link_lengths_[0] <= 0.0 || link_lengths_[1] <= 0.0 ||
      link_lengths_[2] <= 0.0)
    {
      RCLCPP_ERROR(
        get_logger(),
        "Banksia IK measured invalid link lengths: l1=%f l2=%f l3=%f",
        link_lengths_[0],
        link_lengths_[1],
        link_lengths_[2]);
      return false;
    }

    RCLCPP_INFO(
      get_logger(),
      "Banksia IK measured link lengths from AnalysisTree: l1=%.6f l2=%.6f l3=%.6f",
      link_lengths_[0],
      link_lengths_[1],
      link_lengths_[2]);

    return true;
  }

  IKResult get_position_ik(
    const Eigen::Isometry3d & ik_pose,
    const span<const double> ik_seed_state,
    const span<double> solution_state) const override
  {
    if (ik_seed_state.size() < 6) {
      return tl::unexpected(IKFailure::InvalidSeed{
        "Banksia IK expects at least 6 seed joints"});
    }
    if (solution_state.size() < 6) {
      return tl::unexpected(IKFailure::InvalidSeed{
        "Banksia IK requires a pre-allocated output buffer with at least 6 elements"});
    }
    if (!ik_pose.matrix().allFinite()) {
      return tl::unexpected(IKFailure::InvalidTarget{
        "target pose contains NaN or Inf"});
    }

    // Ported from arm_controllers_old/banksia_kinematics_plugin.
    const auto origin = ik_pose.translation();
    const auto x = origin.x();
    const auto y = origin.y();
    const auto z = origin.z();

    const Eigen::Matrix3d rotated_basis = ik_pose.rotation();

    double l1r = link_lengths_[0];
    double l2r = link_lengths_[1];
    double l3  = link_lengths_[2];

    const Eigen::Matrix3d & rxyz = rotated_basis;

    Eigen::Matrix4d t07r = Eigen::Matrix4d::Zero();
    t07r.topLeftCorner<3,3>() = rxyz;

    t07r(3, 3) = 1;
    t07r(0, 3) = x;
    t07r(1, 3) = y;
    t07r(2, 3) = z;

    // Need two DH transforms to get x of end effector frame pointing forwards.
    Eigen::Matrix4d t67 = sub_dh(M_PI / 2, 0, 0, M_PI / 2) * sub_dh(M_PI / 2, l3, 0, 0);
    Eigen::Matrix4d t0_wrist = t07r * t67.inverse();

    double wrist_x = t0_wrist(0, 3);
    double wrist_y = t0_wrist(1, 3);
    double wrist_z = t0_wrist(2, 3);

    double j1 = atan2(wrist_y, wrist_x);
    double l = sqrt(pow(wrist_x, 2) + pow(wrist_y, 2));
    const double cosine_argument =
      (pow(l, 2) + pow(wrist_z, 2) - pow(l1r, 2) - pow(l2r, 2)) / (2 * l1r * l2r);
    if (!std::isfinite(cosine_argument)) {
      return tl::unexpected(IKFailure::BackendError{
        "computed elbow cosine argument is non-finite"});
    }
    if (cosine_argument < -1.0 || cosine_argument > 1.0) {
      return tl::unexpected(IKFailure::OutsideWorkspace{
        "requested pose cannot be reached by the Banksia arm geometry"});
    }

    double j3a = acos(cosine_argument);
    double j3b = -j3a; // expands to -acos((l^2+wrist_z^2-L1r^2-L2r^2)/(2*L1r*L2r)) as per keenan's notes
    double j3bo = -j3b - M_PI / 2;

    double k1b = l1r + l2r * cos(j3b);
    double k2b = l2r * sin(j3b);

    double j2b = atan2(wrist_z, l) - atan2(k2b, k1b);
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
    Eigen::Vector3i indicies = {1, 2, 0};
    Eigen::Matrix3d r37r_shifted = r37r(indicies, Eigen::all);

    // Returns in range [0:pi]x[-pi:pi]x[-pi:pi]. The old implementation chose this
    // over canonical Euler angles so j4 moves less.
    Eigen::Vector3d rpr = r37r_shifted.eulerAngles(0, 1, 0);

    double j4 = -rpr(0);
    double j5 = rpr(1) - M_PI / 2;
    double j6 = rpr(2);

    // Move j4's range from [-pi:0] to [-pi/2:pi/2]. If j4 rotates 180 degrees
    // because of this, j5 and j6 need inverting too.
    if (j4 < -M_PI / 2) {
      j4 = j4 + M_PI;
      j5 = -rpr(1) - M_PI / 2;
      j6 = j6 + (j6 < 0 ? M_PI : -M_PI);
    }

    // The old plugin returned {j1, j2, j4, j5, j6, j3} because MoveIt reordered
    // Banksia's active joint group. This plugin writes the configured controller order.
    assert(solution_state.size() >= 6);
    solution_state[0] = j1;
    solution_state[1] = j2bo + M_PI / 2;
    solution_state[2] = j3bo + j2bo + M_PI / 2;
    solution_state[3] = j4;
    solution_state[4] = j5;
    solution_state[5] = j6;

    for (size_t i = 0; i < 6; ++i) {
      if (!std::isfinite(solution_state[i])) {
        return tl::unexpected(IKFailure::BackendError{
          "Banksia IK produced NaN or Inf joint values"});
      }
    }

    return {};
  }

  // substitutes values into our DH table.
  // see: https://www.notion.so/Inverse-Kinematics-ddfe35179c1f4959850bd28b2195be8a
  // equivalent line: DHs = [cos(the) -sin(the) 0 a; sin(the)*cos(alp) cos(the)*cos(alp) -sin(alp) -sin(alp)*d; sin(the)*sin(alp) cos(the)*sin(alp) cos(alp) cos(alp)*d; 0 0 0 1];
  static Eigen::Matrix4d sub_dh(double alp, double a, double d, double the)
  {
    return Eigen::Matrix4d {
        { cos(the), 			-sin(the), 			0, 			a },
        { sin(the)*cos(alp),	cos(the)*cos(alp), 	-sin(alp), 	-sin(alp)*d },
        { sin(the)*sin(alp),	cos(the)*sin(alp),	cos(alp),	cos(alp)*d },
        { 0,					0,					0,			1 }
    };
  }

private:
  std::array<double, 3> link_lengths_ {0.5052, 0.6193, 0.2225 + 0.021040};
};

} // namespace arm_kinematics

#include <pluginlib/class_list_macros.hpp>

PLUGINLIB_EXPORT_CLASS(arm_kinematics::BanksiaIKPlugin, arm_kinematics::InverseKinematicsPlugin)
