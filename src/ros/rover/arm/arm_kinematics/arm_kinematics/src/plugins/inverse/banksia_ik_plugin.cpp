//
// Created by Bailey Chessum on 15/12/2025.
//

#include "arm_kinematics/inverse/inverse_kinematics_plugin.hpp"

namespace {
  const auto ENDEFFECTOR_BASIS_INVERSE = Eigen::Quaternionf(0.5, -0.5, 0.5, 0.5).toRotationMatrix().inverse();
}

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

  bool get_position_ik(
    const Eigen::Isometry3f & ik_pose,
    const std::vector<double> & ik_seed_state,
    std::vector<double> & solution_state) const override
  {
    assert(ik_seed_state.size() >= 6);

    // Adapted from Abby's code
    const auto origin = ik_pose.translation();
    const auto x = origin.x();
    const auto y = origin.y();
    const auto z = origin.z();

    const Eigen::Matrix3f rotated_basis = ik_pose.rotation() * ENDEFFECTOR_BASIS_INVERSE;

    double l1r = link_lengths_[0];
    double l2r = link_lengths_[1];
    double l3  = link_lengths_[2];

    const Eigen::Matrix3f & rxyz = rotated_basis;

    Eigen::Matrix4f t07r = Eigen::Matrix4f::Zero();
    t07r.topLeftCorner<3,3>() = rxyz;

    t07r(3, 3) = 1;
    t07r(0, 3) = x;
    t07r(1, 3) = y;
    t07r(2, 3) = z;

    Eigen::Matrix4f t67 = to_matrix4(
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, l3,
      0, 0, 0, 1
    );
    Eigen::Matrix4f t0_wrist = t07r * t67.inverse();

    double wrist_x = t0_wrist(0, 3);
    double wrist_y = t0_wrist(1, 3);
    double wrist_z = t0_wrist(2, 3);

    double j1 = atan2(wrist_y, wrist_x);
    double l = sqrt(pow(wrist_x, 2) + pow(wrist_y, 2));
    double j3a = acos((pow(l, 2) + pow(wrist_z, 2) - pow(l1r, 2) - pow(l2r, 2)) / (2 * l1r * l2r));
    double j3b = -j3a; // expands to -acos((l^2+wrist_z^2-L1r^2-L2r^2)/(2*L1r*L2r)) as per keenan's notes
    // double j3ao = j3a + M_PI / 2;
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
    Eigen::Matrix3d r07r = t07r.topLeftCorner<3, 3>().cast<double>(); // see above
    Eigen::Matrix3d r37r = r03_wrist.inverse() * r07r;

    double j4 = atan2(r37r(1, 2), r37r(0, 2));
    double j5 = atan2(-r37r(2, 2), r37r(0, 2) / cos(j4));
    double j6 = atan2(-r37r(2, 1) / cos(j5), r37r(2, 0) / cos(j5));

    // Old implementation needed to be in the same order as when they get put in a joint group
    // std::array<double, 6> new_joints = { j1, j2bo, -j4, j5, j6, j3bo + j2bo };

    // New implementation allows for us to operate on an arbitrary joint order of our choosing,
    // with any reordering being handled by joint_map
    assert(solution_state.size() >= 6);
    solution_state[0] = j1;
    solution_state[1] = j2bo;
    solution_state[2] = j3bo + j2bo;
    solution_state[3] = -j4;
    solution_state[4] = j5;
    solution_state[5] = j6;

    return true;
  }

  static Eigen::Matrix4f to_matrix4(
      float r00, float r01, float r02, float tx,
      float r10, float r11, float r12, float ty,
      float r20, float r21, float r22, float tz,
      float b0 = 0,  float b1 = 0,  float b2 = 0, float b3 = 1)
  {
    Eigen::Matrix4f m;
    m << r00, r01, r02, tx,
         r10, r11, r12, ty,
         r20, r21, r22, tz,
         b0,  b1,  b2,  b3;
    return m;
  }

  static Eigen::Isometry3f to_isometry(
      float r00, float r01, float r02, float tx,
      float r10, float r11, float r12, float ty,
      float r20, float r21, float r22, float tz)
  {
    Eigen::Matrix4f m;
    m << r00, r01, r02, tx,
         r10, r11, r12, ty,
         r20, r21, r22, tz,
         0.0, 0.0, 0.0, 1.0;

    Eigen::Isometry3f T(m);   // or: Eigen::Isometry3f T = m;
    return T;
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
  std::array<double, 3> link_lengths_ {0.5, 0.41799975417, 0.417};
};

} // namespace arm_kinematics

#include <pluginlib/class_list_macros.hpp>

PLUGINLIB_EXPORT_CLASS(arm_kinematics::BanksiaIKPlugin, arm_kinematics::InverseKinematicsPlugin)
