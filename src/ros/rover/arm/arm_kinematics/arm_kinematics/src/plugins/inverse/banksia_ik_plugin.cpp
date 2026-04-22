//
// Created by Bailey Chessum on 15/12/2025.
//

#include "arm_kinematics/inverse/inverse_kinematics_plugin.hpp"

#include <array>
#include <cassert>
#include <cmath>

namespace arm_kinematics {

/**
 * IK plugin for Taipan with the original (bad) carbon fibre wrist
 */
class BanksiaIKPlugin : public InverseKinematicsPlugin {
public:

  bool on_initialize() override {
    // TODO: Auto measure link_lengths_ using get_robot_model().get_analysis_tree() ?

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
