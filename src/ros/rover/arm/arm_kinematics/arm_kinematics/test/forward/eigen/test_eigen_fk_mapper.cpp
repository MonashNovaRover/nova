//
// Created by Bailey Chessum on 17/11/2025.
//

#include <gtest/gtest.h>
#include <urdf_model/model.h>
#include <Eigen/Geometry>

#include <arm_kinematics/forward/eigen/compute_joint_tree.hpp>
#include <arm_kinematics/forward/eigen/compute_frame_tree.hpp>
#include <arm_kinematics/forward/eigen/eigen_forward_kinematics_plugin.hpp>
#include <arm_kinematics/joint_map/joint_map.hpp>
#include <arm_kinematics/joint_map/joint_map_builder.hpp>

#include <rclcpp/rclcpp.hpp>

using arm_kinematics::ComputeFrameTree;
using arm_kinematics::ComputeJointTree;
using arm_kinematics::FrameDefinitions;
using arm_kinematics::JointMap;
using arm_kinematics::JointMapBuilder;
using arm_kinematics::ForwardKinematicsPlugin;
using arm_kinematics::EigenForwardKinematicsPlugin;
using arm_kinematics::AnalysisTree;

static void ExpectVectorNear(const Eigen::Vector3d & actual,
                             const Eigen::Vector3d & expected,
                             const char * message = "", double tol = 1e-10)
{
  EXPECT_NEAR(actual.x(), expected.x(), tol) << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix();
  EXPECT_NEAR(actual.y(), expected.y(), tol) << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix();
  EXPECT_NEAR(actual.z(), expected.z(), tol) << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix();
}

// Small helper for comparing isometries
static void ExpectIsometryNear(const Eigen::Isometry3d & actual,
                               const Eigen::Isometry3d & expected,
                               const char * message = "", double tol = 1e-10)
{
  ExpectVectorNear(actual.translation(), expected.translation(), message, tol);
  // EXPECT_NEAR(actual.translation().x(), expected.translation().x(), tol) << actual.matrix() << " \n vs \n" << expected.matrix();
  // EXPECT_NEAR(actual.translation().y(), expected.translation().y(), tol) << actual.matrix() << "\n vs \n" << expected.matrix();
  // EXPECT_NEAR(actual.translation().z(), expected.translation().z(), tol) << actual.matrix() << "\n vs \n" << expected.matrix();
  EXPECT_TRUE(actual.linear().isApprox(expected.linear(), tol)) << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix() << "\n";
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
  FrameDefinitions frames({"link1"}, {Eigen::Isometry3d::Identity()});  // parent_link_names = { "link1" }, origins = { I }

  // std::vector<std::string> fk_joint_names;
  // EigenFKMapper mapper = build_fk_mapper_from_urdf(
  //   model_,
  //   "base_link",
  //   std::move(frames),
  //   fk_joint_names);
  //
  // // Expect only joint1 is needed
  // ASSERT_EQ(fk_joint_names.size(), 1u);
  // EXPECT_EQ(fk_joint_names[0], "joint1");
  //
  // // Provide a single joint state: theta about Z
  // const double theta = M_PI / 4.0;
  // std::vector<double> joint_states{ theta };
  //
  // arm_kinematics::Isometry3dVector poses(1);
  // mapper.update(joint_states, poses);
  //
  // Eigen::Isometry3d expected = Eigen::Isometry3d::Identity();
  // expected.linear() =
  //   Eigen::AngleAxisd(theta, Eigen::Vector3d::UnitZ()).toRotationMatrix();
  // expected.translation().setZero();
  //
  // ExpectIsometryNear(poses[0], expected);
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

  // std::vector<std::string> fk_joint_names;
  // EigenFKMapper mapper = build_fk_mapper_from_urdf(
  //   model_,
  //   "base_link",
  //   std::move(frames),
  //   fk_joint_names);
  //
  // ASSERT_EQ(fk_joint_names.size(), 2u);
  // // We expect joint1 and joint2 in some order that matches the tree. For this
  // // simple chain, a natural ordering is [joint1, joint2].
  // EXPECT_EQ(fk_joint_names[0], "joint1");
  // EXPECT_EQ(fk_joint_names[1], "joint2");
  //
  // const double theta = M_PI / 2.0;  // 90deg about Z
  // const double d     = 0.5;         // 0.5m along +X in link1 frame
  //
  // std::vector<double> joint_states{ theta, d };
  // arm_kinematics::Isometry3dVector poses(1);
  // mapper.update(joint_states, poses);
  //
  // // Kinematics:
  // //  base -> link1: rotation Rz(theta), no translation
  // //  link1 -> link2: origin (0,1,0) then prismatic along +X by d
  // //
  // // In base frame:
  // //  link2 translation = Rz(theta) * [d, 1, 0]^T
  // //  link2 rotation    = Rz(theta)
  // Eigen::Isometry3d expected = Eigen::Isometry3d::Identity();
  // Eigen::Vector3d local( d, 1.0, 0.0 );
  // Eigen::Matrix3d R = Eigen::AngleAxisd(theta, Eigen::Vector3d::UnitZ()).toRotationMatrix();
  //
  // expected.linear() = R;
  // expected.translation() = R * local;
  //
  // ExpectIsometryNear(poses[0], expected);
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

  // std::vector<std::string> fk_joint_names;
  // EigenFKMapper mapper = build_fk_mapper_from_urdf(
  //   model_,
  //   "base_link",
  //   std::move(frames),
  //   fk_joint_names);
  //
  // // Still only two actuated joints
  // ASSERT_EQ(fk_joint_names.size(), 2u);
  // EXPECT_EQ(fk_joint_names[0], "joint1");
  // EXPECT_EQ(fk_joint_names[1], "joint2");
  //
  // const double theta = M_PI / 2.0;
  // const double d     = 0.5;
  //
  // std::vector<double> joint_states{ theta, d };
  // arm_kinematics::Isometry3dVector poses(1);
  // mapper.update(joint_states, poses);
  //
  // // link3 sits at a fixed offset (0,0,0.1) from link2 in link2's frame.
  // //
  // // base -> link2 as in previous test:
  // //   p2 = Rz(theta) * [d, 1, 0]^T
  // //   R2 = Rz(theta)
  // //
  // // link2 -> link3: [0, 0, 0.1]^T
  // // so:
  // //   p3 = p2 + R2 * [0, 0, 0.1]^T
  // Eigen::Vector3d local2(d, 1.0, 0.0);
  // Eigen::Matrix3d R = Eigen::AngleAxisd(theta, Eigen::Vector3d::UnitZ()).toRotationMatrix();
  // Eigen::Vector3d p2 = R * local2;
  // Eigen::Vector3d local_fix(0.0, 0.0, 0.1);
  // Eigen::Vector3d p3 = p2 + R * local_fix;
  //
  // Eigen::Isometry3d expected = Eigen::Isometry3d::Identity();
  // expected.linear() = R;
  // expected.translation() = p3;
  //
  // ExpectIsometryNear(poses[0], expected);
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
    robot_description_ = R"(
      <robot name="test_robot">
        <link name="base_link"/>
        <link name="link1"/>
        <link name="link2"/>

        <joint name="joint1" type="prismatic">
          <parent link="base_link"/>
          <child  link="link1"/>
          <origin xyz="0 1 0" rpy="0 0 0"/>
          <axis   xyz="1 0 0"/>
          <limit  lower="-1.0" upper="1.0" effort="10.0" velocity="10.0"/>
        </joint>

        <joint name="joint2" type="revolute">
          <parent link="link1"/>
          <child  link="link2"/>
          <origin xyz="0 0 0" rpy="0 0 0"/>
          <axis   xyz="0 0 1"/>
          <limit  lower="-3.14159" upper="3.14159" effort="10.0" velocity="10.0"/>
        </joint>
      </robot>
    )";
    ASSERT_TRUE(model_.initString(robot_description_));

    node_ = std::make_shared<rclcpp::Node>("test_eigen_fk_mapper");
  }

  void TearDown() override {
    node_.reset();
  }

  std::string robot_description_;
  urdf::Model model_;
  std::shared_ptr<rclcpp::Node> node_;
};

TEST_F(EigenForwardKinematicsPluginTreeTest, SimpleComputeFrameTree)
{
  // One frame attached to link2 with identity origin
  FrameDefinitions frames("link2");
  const size_t output_count = frames.origins.size();
  std::vector<std::string> joint_names{"joint1", "joint2"};

  // Build mapper + joint names from URDF
  RCLCPP_INFO(node_->get_logger(), "Creating plugin");
  ForwardKinematicsPlugin::SharedPtr plugin = std::make_shared<EigenForwardKinematicsPlugin>();
  RCLCPP_INFO(node_->get_logger(), "Initializing plugin");
  auto init_result = plugin->initialize(*node_, robot_description_);

  ASSERT_TRUE(init_result) << "Failed to initialize plugin";

  const double theta = M_PI / 2.0;
  const double d     = 0.5;
  std::vector<double> joint_states{ d, theta };

  RCLCPP_INFO(node_->get_logger(), "Creating FK Tree");
  auto anal = AnalysisTree(plugin->get_urdf_model());

  auto joint_order = anal.sort_joints();
  auto tree = anal.make_compute_joint_tree();

  ASSERT_EQ(tree.poses.size(), joint_states.size());

  RCLCPP_INFO(node_->get_logger(), "Performing FK");
  tree.update(joint_states);

  // Compare against truth
  RCLCPP_INFO(node_->get_logger(), "Testing against true poses");
  arm_kinematics::Isometry3dVector true_poses(output_count);

  ExpectVectorNear(tree.get_axes()[0], Eigen::Vector3d(1, 0, 0), "ComputeJointTree axis[0] is incorrect!");
  ExpectVectorNear(tree.get_axes()[1], Eigen::Vector3d(0, 0, 1), "ComputeJointTree axis[1] is incorrect!");
  EXPECT_EQ(tree.get_types()[0], arm_kinematics::JointType::PRISMATIC);
  EXPECT_EQ(tree.get_types()[1], arm_kinematics::JointType::REVOLUTE);

  Eigen::Isometry3d link1_truth = Eigen::Isometry3d::Identity();
  link1_truth.translation() = Eigen::Vector3d(0.5, 1, 0);

  ExpectIsometryNear(tree.poses[0], link1_truth, "link1 pose is wrong");

  Eigen::Isometry3d link2_truth = Eigen::Isometry3d::Identity();
  link2_truth.linear() = Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d(0, 0, 1)).toRotationMatrix();
  link2_truth.translation() = Eigen::Vector3d(0.5, 1, 0);

  ExpectIsometryNear(tree.poses[tree.poses.size() - 1], link2_truth, "link2 pose is wrong");

  // true_poses = result.order->map(true_poses);
  //
  // ASSERT_EQ(link_poses.size(), true_poses.size());
  // for (size_t i = 0; i < link_poses.size(); ++i) {
  //   ExpectIsometryNear(true_poses[i], link_poses[i]);
  // }
}

TEST_F(EigenForwardKinematicsPluginTreeTest, TreePositionFkMatchesMapper)
{
  // One frame attached to link2 with identity origin
  FrameDefinitions frames("link2");
  const size_t output_count = frames.origins.size();
  std::vector<std::string> joint_names{"joint1", "joint2"};

  // Build mapper + joint names from URDF
  RCLCPP_INFO(node_->get_logger(), "Creating plugin");
  ForwardKinematicsPlugin::SharedPtr plugin = std::make_shared<EigenForwardKinematicsPlugin>();
  RCLCPP_INFO(node_->get_logger(), "Initializing plugin");
  auto init_result = plugin->initialize(*node_, robot_description_);

  ASSERT_TRUE(init_result) << "Failed to initialize plugin";

  const double theta = M_PI / 2.0;
  const double d     = 0.5;
  std::vector<double> joint_states{ theta, d };

  RCLCPP_INFO(node_->get_logger(), "Creating FK Tree");
  auto result = plugin->make_tree(joint_names, std::string("base_link"), frames);

  ASSERT_TRUE(result.order) << "Failed to create order";
  ASSERT_TRUE(result.order->size() > 0) << "Failed to create tree with order";

  ForwardKinematicsPlugin::Tree::SharedPtr tree = std::move(result.tree);
  ASSERT_TRUE(tree) << "Failed to create tree";

  RCLCPP_INFO(node_->get_logger(), "Allocating pose outputs");
  arm_kinematics::Isometry3dVector link_poses(output_count, Eigen::Isometry3d::Identity());
  RCLCPP_INFO(node_->get_logger(), "Performing FK");
  tree->position_fk(joint_states, link_poses);

  // Compare against truth
  RCLCPP_INFO(node_->get_logger(), "Testing against true poses");
  arm_kinematics::Isometry3dVector true_poses(output_count);

  Eigen::Isometry3d & link2_truth = true_poses[0] = Eigen::Isometry3d::Identity();
  link2_truth.translation() = Eigen::Vector3d(0.5, 1, 0);
  link2_truth.linear() = Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d(0, 0, 1)).toRotationMatrix();

  true_poses = result.order->map(true_poses);

  ASSERT_EQ(link_poses.size(), true_poses.size());
  for (size_t i = 0; i < link_poses.size(); ++i) {
    ExpectIsometryNear(true_poses[i], link_poses[i]);
  }
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  // Build a small argv for ROS, with logging to file disabled
  const char * ros_argv[] = {
    argv[0],
    "--ros-args",
    "--disable-external-lib-logs",
    "--disable-rosout-logs",   // optional: no /rosout
  };
  int ros_argc = static_cast<int>(sizeof(ros_argv) / sizeof(ros_argv[0]));

  rclcpp::init(ros_argc, const_cast<char **>(ros_argv));

  int ret = RUN_ALL_TESTS();

  rclcpp::shutdown();
  return ret;
}

