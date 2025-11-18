//
// Created by Bailey Chessum on 17/11/2025.
//

#include <gtest/gtest.h>
#include <urdf_model/model.h>
#include <Eigen/Geometry>

#include <arm_kinematics/forward/eigen/eigen_fk_tree.hpp>
#include <arm_kinematics/forward/eigen/eigen_fk_mapper.hpp>
#include <arm_kinematics/forward/eigen/eigen_forward_kinematics_plugin.hpp>
#include <arm_kinematics/joint_map/joint_map.hpp>
#include <arm_kinematics/joint_map/joint_map_builder.hpp>
#include "arm_kinematics/forward/eigen/eigen_fk_detail.hpp"

using arm_kinematics::EigenFKTree;
using arm_kinematics::EigenFKMapper;
using arm_kinematics::FrameDefinitions;
using arm_kinematics::JointMap;
using arm_kinematics::JointMapBuilder;
using arm_kinematics::ForwardKinematicsPlugin;
using arm_kinematics::EigenForwardKinematicsPlugin;
using arm_kinematics::detail::build_fk_mapper_from_urdf;

// Small helper for comparing isometries
static void ExpectIsometryNear(const Eigen::Isometry3d & actual,
                               const Eigen::Isometry3d & expected,
                               double tol = 1e-10)
{
  EXPECT_NEAR(actual.translation().x(), expected.translation().x(), tol);
  EXPECT_NEAR(actual.translation().y(), expected.translation().y(), tol);
  EXPECT_NEAR(actual.translation().z(), expected.translation().z(), tol);
  EXPECT_TRUE(actual.linear().isApprox(expected.linear(), tol));
}

// ---------------------------------------------------------------------------
// Test fixture: builds a small URDF model with fixed + actuated joints.
//
//  base_link
//    └─ joint1 (revolute Z) -> link1
//         └─ joint2 (prismatic X, origin y=1) -> link2
//              └─ joint3 (fixed, origin z=0.1) -> link3
//
// We use this to test:
//  * FK tree over {joint1, joint2} only (no fixed joint3).
//  * Mapping frames attached to link1, link2, and link3.
// ---------------------------------------------------------------------------

class EigenFKMapperTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    const std::string urdf_xml = R"(
      <robot name="test_robot">
        <link name="base_link"/>
        <link name="link1"/>
        <link name="link2"/>
        <link name="link3"/>

        <!-- Revolute joint about Z between base_link and link1 -->
        <joint name="joint1" type="revolute">
          <parent link="base_link"/>
          <child  link="link1"/>
          <origin xyz="0 0 0" rpy="0 0 0"/>
          <axis   xyz="0 0 1"/>
          <limit  lower="-3.14159" upper="3.14159" effort="10.0" velocity="10.0"/>
        </joint>

        <!-- Prismatic joint along X between link1 and link2, offset in Y -->
        <joint name="joint2" type="prismatic">
          <parent link="link1"/>
          <child  link="link2"/>
          <origin xyz="0 1 0" rpy="0 0 0"/>
          <axis   xyz="1 0 0"/>
          <limit  lower="-1.0" upper="1.0" effort="10.0" velocity="10.0"/>
        </joint>

        <!-- Fixed joint from link2 to link3, offset along Z -->
        <joint name="joint3" type="fixed">
          <parent link="link2"/>
          <child  link="link3"/>
          <origin xyz="0 0 0.1" rpy="0 0 0"/>
        </joint>
      </robot>
    )";


    ASSERT_TRUE(model_.initString(urdf_xml))
      << "Failed to initialise URDF model from string";
  }

  urdf::Model model_;
};

// ---------------------------------------------------------------------------
// 1) Basic check: build mapper for a single frame on link1 (identity offset).
//    Ensures:
//    * Only actuated joints are used.
//    * joint_names order is { "joint1" }.
//    * FK for link1 is correct for a simple rotation about Z.
// ---------------------------------------------------------------------------

TEST_F(EigenFKMapperTest, SingleFrameOnLink1)
{
  // FrameDefinitions: one frame attached directly to link1 with identity origin
  FrameDefinitions frames("link1");  // parent_link_names = { "link1" }, origins = { I }

  std::vector<std::string> fk_joint_names;
  EigenFKMapper mapper = build_fk_mapper_from_urdf(
    model_,
    "base_link",
    std::move(frames),
    fk_joint_names);

  // Expect only joint1 is needed
  ASSERT_EQ(fk_joint_names.size(), 1u);
  EXPECT_EQ(fk_joint_names[0], "joint1");

  // Provide a single joint state: theta about Z
  const double theta = M_PI / 4.0;
  std::vector<double> joint_states{ theta };

  arm_kinematics::Isometry3dVector poses(1);
  mapper.update(joint_states, poses);

  Eigen::Isometry3d expected = Eigen::Isometry3d::Identity();
  expected.linear() =
    Eigen::AngleAxisd(theta, Eigen::Vector3d::UnitZ()).toRotationMatrix();
  expected.translation().setZero();

  ExpectIsometryNear(poses[0], expected);
}

// ---------------------------------------------------------------------------
// 2) Frame on link2 (child of prismatic joint).
//    FrameDefinitions: parent_link = "link2", origin = I.
//    Checks that:
//      * joint_names = { "joint1", "joint2" } in some consistent order.
//      * FK includes revolute + prismatic behaviour, with the correct offset
//        from link1 to link2 (y = 1.0 + x translation along joint2 axis).
// ---------------------------------------------------------------------------

TEST_F(EigenFKMapperTest, FrameOnLink2WithRevoluteAndPrismatic)
{
  FrameDefinitions frames("link2");

  std::vector<std::string> fk_joint_names;
  EigenFKMapper mapper = build_fk_mapper_from_urdf(
    model_,
    "base_link",
    std::move(frames),
    fk_joint_names);

  ASSERT_EQ(fk_joint_names.size(), 2u);
  // We expect joint1 and joint2 in some order that matches the tree. For this
  // simple chain, a natural ordering is [joint1, joint2].
  EXPECT_EQ(fk_joint_names[0], "joint1");
  EXPECT_EQ(fk_joint_names[1], "joint2");

  const double theta = M_PI / 2.0;  // 90deg about Z
  const double d     = 0.5;         // 0.5m along +X in link1 frame

  std::vector<double> joint_states{ theta, d };
  arm_kinematics::Isometry3dVector poses(1);
  mapper.update(joint_states, poses);

  // Kinematics:
  //  base -> link1: rotation Rz(theta), no translation
  //  link1 -> link2: origin (0,1,0) then prismatic along +X by d
  //
  // In base frame:
  //  link2 translation = Rz(theta) * [d, 1, 0]^T
  //  link2 rotation    = Rz(theta)
  Eigen::Isometry3d expected = Eigen::Isometry3d::Identity();
  Eigen::Vector3d local( d, 1.0, 0.0 );
  Eigen::Matrix3d R = Eigen::AngleAxisd(theta, Eigen::Vector3d::UnitZ()).toRotationMatrix();

  expected.linear() = R;
  expected.translation() = R * local;

  ExpectIsometryNear(poses[0], expected);
}

// ---------------------------------------------------------------------------
// 3) Frame on link3, which is reachable only through a FIXED joint3 from link2.
//    We request a frame attached to link3 with identity offset.
//    Checks that:
//      * joint_names still only reference {joint1, joint2} (no joint3).
//      * mapper offset correctly bakes the fixed joint into the result.
// ---------------------------------------------------------------------------

TEST_F(EigenFKMapperTest, FrameOnLink3BakesFixedJointIntoOffset)
{
  FrameDefinitions frames("link3");

  std::vector<std::string> fk_joint_names;
  EigenFKMapper mapper = build_fk_mapper_from_urdf(
    model_,
    "base_link",
    std::move(frames),
    fk_joint_names);

  // Still only two actuated joints
  ASSERT_EQ(fk_joint_names.size(), 2u);
  EXPECT_EQ(fk_joint_names[0], "joint1");
  EXPECT_EQ(fk_joint_names[1], "joint2");

  const double theta = M_PI / 2.0;
  const double d     = 0.5;

  std::vector<double> joint_states{ theta, d };
  arm_kinematics::Isometry3dVector poses(1);
  mapper.update(joint_states, poses);

  // link3 sits at a fixed offset (0,0,0.1) from link2 in link2's frame.
  //
  // base -> link2 as in previous test:
  //   p2 = Rz(theta) * [d, 1, 0]^T
  //   R2 = Rz(theta)
  //
  // link2 -> link3: [0, 0, 0.1]^T
  // so:
  //   p3 = p2 + R2 * [0, 0, 0.1]^T
  Eigen::Vector3d local2(d, 1.0, 0.0);
  Eigen::Matrix3d R = Eigen::AngleAxisd(theta, Eigen::Vector3d::UnitZ()).toRotationMatrix();
  Eigen::Vector3d p2 = R * local2;
  Eigen::Vector3d local_fix(0.0, 0.0, 0.1);
  Eigen::Vector3d p3 = p2 + R * local_fix;

  Eigen::Isometry3d expected = Eigen::Isometry3d::Identity();
  expected.linear() = R;
  expected.translation() = p3;

  ExpectIsometryNear(poses[0], expected);
}

// ---------------------------------------------------------------------------
// 4) Integration: EigenForwardKinematicsPlugin::Tree
//    Uses:
//     * build_fk_mapper_from_urdf for mapping
//     * JointMap with identity mapping (no mimic/transmission)
//    Then checks that Tree::position_fk matches mapper's result.
// ---------------------------------------------------------------------------

class EigenForwardKinematicsPluginTreeTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    const std::string urdf_xml = R"(
      <robot name="test_robot">
        <link name="base_link"/>
        <link name="link1"/>
        <link name="link2"/>

        <joint name="joint1" type="revolute">
          <parent link="base_link"/>
          <child  link="link1"/>
          <origin xyz="0 0 0" rpy="0 0 0"/>
          <axis   xyz="0 0 1"/>
          <limit  lower="-3.14159" upper="3.14159" effort="10.0" velocity="10.0"/>
        </joint>

        <joint name="joint2" type="prismatic">
          <parent link="link1"/>
          <child  link="link2"/>
          <origin xyz="0 1 0" rpy="0 0 0"/>
          <axis   xyz="1 0 0"/>
          <limit  lower="-1.0" upper="1.0" effort="10.0" velocity="10.0"/>
        </joint>
      </robot>
    )";
    ASSERT_TRUE(model_.initString(urdf_xml));
  }

  urdf::Model model_;
};

TEST_F(EigenForwardKinematicsPluginTreeTest, TreePositionFkMatchesMapper)
{
  // One frame attached to link2 with identity origin
  FrameDefinitions frames("link2");
  const size_t output_count = frames.origins.size();

  // Build mapper + joint names from URDF
  std::vector<std::string> mapper_joint_names;
  EigenFKMapper mapper = build_fk_mapper_from_urdf(
    model_,
    "base_link",
    FrameDefinitions(frames),  // copy
    mapper_joint_names);

  // Identity JointMap: inputs are already in mapper_joint_names order
  JointMap identity_map = JointMap::identity(mapper_joint_names.size());

  // Build the Tree
  EigenForwardKinematicsPlugin::TreeImpl tree(
    output_count,
    mapper,
    identity_map);

  const double theta = M_PI / 2.0;
  const double d     = 0.5;
  std::vector<double> joint_states{ theta, d };

  arm_kinematics::Isometry3dVector link_poses(output_count);
  tree.position_fk(joint_states, link_poses);

  // Compare against direct mapper usage
  arm_kinematics::Isometry3dVector ref_poses(output_count);
  mapper.update(joint_states, ref_poses);

  ASSERT_EQ(link_poses.size(), ref_poses.size());
  for (size_t i = 0; i < link_poses.size(); ++i) {
    ExpectIsometryNear(link_poses[i], ref_poses[i]);
  }
}
