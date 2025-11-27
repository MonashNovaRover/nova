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

Eigen::Isometry3d to_isometry(
    double r00, double r01, double r02, double tx,
    double r10, double r11, double r12, double ty,
    double r20, double r21, double r22, double tz)
{
  Eigen::Matrix4d m;
  m << r00, r01, r02, tx,
       r10, r11, r12, ty,
       r20, r21, r22, tz,
       0.0, 0.0, 0.0, 1.0;

  Eigen::Isometry3d T(m);   // or: Eigen::Isometry3d T = m;
  return T;
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
          <origin xyz="0 0 1" rpy="0 0 0"/>
          <axis   xyz="0 0 1"/>
          <limit  lower="-3.14159" upper="3.14159" effort="10.0" velocity="10.0"/>
        </joint>
      </robot>
    )";
    ASSERT_TRUE(model_.initString(robot_description_));

    node_ = std::make_shared<rclcpp::Node>("test_eigen_fk_mapper");
    logger_ = node_->get_logger();
  }

  void TearDown() override {
    node_.reset();
  }

  std::string robot_description_;
  urdf::Model model_;
  std::shared_ptr<rclcpp::Node> node_;
  rclcpp::Logger logger_ = rclcpp::get_logger("not_initialized");
};

TEST_F(EigenForwardKinematicsPluginTreeTest, SimpleUrdfComputeJointTree)
{
  // One frame attached to link2 with identity origin
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

  ExpectIsometryNear(anal.get_joints()[0].origin, Eigen::Isometry3d::Identity(),
    "dummy root origin is incorrect");
  ExpectIsometryNear(anal.get_joints()[1].origin, to_isometry(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 1.0,
    0.0, 0.0, 1.0, 0.0
  ), "joint 1 origin is incorrect");
  ExpectIsometryNear(anal.get_joints()[2].origin, to_isometry(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 1.0
  ), "joint 2 origin is incorrect");

  auto joint_order = anal.sort_joints();

  ExpectIsometryNear(anal.get_joints()[0].origin, Eigen::Isometry3d::Identity(),
    "dummy root origin is incorrect");
  ExpectIsometryNear(anal.get_joints()[1].origin, to_isometry(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 1.0,
    0.0, 0.0, 1.0, 0.0
  ), "joint 1 origin is incorrect");
  ExpectIsometryNear(anal.get_joints()[2].origin, to_isometry(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 1.0
  ), "joint 2 origin is incorrect");

  auto tree = anal.make_compute_joint_tree();

  ASSERT_EQ(tree.poses.size(), joint_states.size()) << "Wrong number of joints in the tree. Is there redundancy?";
  ASSERT_EQ(tree.get_root_relative_count(), 1) << "Wrong root relative joint count!";

  RCLCPP_INFO(node_->get_logger(), "Performing FK");
  tree.update(joint_states);

  // Compare against truth
  RCLCPP_INFO(node_->get_logger(), "Testing against true poses");

  ExpectVectorNear(tree.get_axes()[0], Eigen::Vector3d(1, 0, 0), "ComputeJointTree axis[0] is incorrect!");
  ExpectVectorNear(tree.get_axes()[1], Eigen::Vector3d(0, 0, 1), "ComputeJointTree axis[1] is incorrect!");
  EXPECT_EQ(tree.get_types()[0], arm_kinematics::JointType::PRISMATIC);
  EXPECT_EQ(tree.get_types()[1], arm_kinematics::JointType::REVOLUTE);
  ExpectIsometryNear(tree.get_origins()[0], to_isometry(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 1.0,
    0.0, 0.0, 1.0, 0.0
  ), "origin 0 is incorrect");
  ExpectIsometryNear(tree.get_origins()[1], to_isometry(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 1.0
  ), "origin 1 is incorrect");

  Eigen::Isometry3d link1_truth = Eigen::Isometry3d::Identity();
  link1_truth.translation() = Eigen::Vector3d(0.5, 1, 0);

  ExpectIsometryNear(tree.poses[0], link1_truth, "link1 pose is wrong");

  Eigen::Isometry3d link2_truth = Eigen::Isometry3d::Identity();
  link2_truth.linear() = Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d(0, 0, 1)).toRotationMatrix();
  link2_truth.translation() = Eigen::Vector3d(0.5, 1, 1);

  ExpectIsometryNear(tree.poses[1], link2_truth, "link2 pose is wrong");
}

TEST_F(EigenForwardKinematicsPluginTreeTest, SimpleUrdfEigenFKPluginTree)
{
  // One frame attached to link2 with identity origin
  FrameDefinitions frames = {std::vector<std::string>{"link1", "link2"}};
  const size_t output_count = frames.origins.size();
  std::vector<std::string> joint_names{"joint1", "joint2"};

  // Build mapper + joint names from URDF
  RCLCPP_INFO(node_->get_logger(), "Creating plugin");
  EigenForwardKinematicsPlugin::SharedPtr plugin = std::make_shared<EigenForwardKinematicsPlugin>();
  RCLCPP_INFO(node_->get_logger(), "Initializing plugin");
  auto init_result = plugin->initialize(*node_, robot_description_);

  ASSERT_TRUE(init_result) << "Failed to initialize plugin";

  const double theta = M_PI / 2.0;
  const double d     = 0.5;
  std::vector<double> joint_states{ d, theta };

  auto weird_joint_map = plugin->get_joint_map_builder().build({"joint1", "joint2"}, {"joint1", "joint2", "joint1", "joint 1", "joint 2"});
  std::vector<double> mapped_joint_states_weird(weird_joint_map.output_count);
  weird_joint_map.map(joint_states, mapped_joint_states_weird);

  RCLCPP_INFO(node_->get_logger(), "Creating FK Tree");
  auto result = plugin->make_tree(joint_names, std::string("base_link"), frames);

  ASSERT_TRUE(result.order) << "Failed to create order";
  ASSERT_TRUE(result.order->size() > 0) << "Failed to create tree with order";

  EigenForwardKinematicsPlugin::TreeImpl::SharedPtr tree =
    std::dynamic_pointer_cast<EigenForwardKinematicsPlugin::TreeImpl>(result.tree);
  ASSERT_TRUE(tree) << "Failed to create tree";


  std::vector<double> mapped_joint_states(tree->get_joint_map().output_count);
  tree->get_joint_map().map(joint_states, mapped_joint_states);
  EXPECT_NEAR(mapped_joint_states[0], joint_states[0], 1e-10) << "joint 0 value mapped incorrectly";
  EXPECT_NEAR(mapped_joint_states[1], joint_states[1], 1e-10) << "joint 1 value mapped incorrectly";

  EXPECT_NEAR(tree->get_joint_map().multipliers[0], 1, 1e-10) << "wrong joint 0 multiplier";
  EXPECT_NEAR(tree->get_joint_map().multipliers[1], 1, 1e-10) << "wrong joint 1 multiplier";

  EXPECT_NEAR(tree->get_joint_map().offsets[0], 0, 1e-10) << "wrong joint 0 offset";
  EXPECT_NEAR(tree->get_joint_map().offsets[1], 0, 1e-10) << "wrong joint 1 offset";

  EXPECT_EQ(tree->get_joint_map().sources[0], 0) << "wrong joint 0 source";
  EXPECT_EQ(tree->get_joint_map().sources[1], 1) << "wrong joint 1 source";

  EXPECT_EQ(tree->get_tree().get_tree().poses.size(), 2) << "joint tree poses are the wrong size";
  EXPECT_EQ(tree->get_joint_map().input_count, 2) << "joint map is the wrong size";
  EXPECT_EQ(tree->get_joint_map().output_count, 2) << "joint map is the wrong size";
  EXPECT_EQ(tree->get_mapped_joint_states().size(), 2) << "mapped joint states are the wrong size";

  EXPECT_EQ(tree->get_tree().get_parents()[0], 0) << "frame 0 has the wrong parent";
  EXPECT_EQ(tree->get_tree().get_parents()[1], 1) << "frame 1 has the wrong parent";

  RCLCPP_INFO(node_->get_logger(), "Allocating pose outputs");
  arm_kinematics::Isometry3dVector link_poses(output_count, Eigen::Isometry3d::Identity());
  RCLCPP_INFO(node_->get_logger(), "Performing FK");

  tree->position_fk(joint_states, link_poses);

  EXPECT_NEAR(tree->get_mapped_joint_states()[0], joint_states[0], 1e-10) << "joint 0 value mapped incorrectly";
  EXPECT_NEAR(tree->get_mapped_joint_states()[1], joint_states[1], 1e-10) << "joint 1 value mapped incorrectly";


  // Compare against truth
  RCLCPP_INFO(node_->get_logger(), "Testing against true poses");

  Eigen::Isometry3d link1_truth = Eigen::Isometry3d::Identity();
  link1_truth.translation() = Eigen::Vector3d(0.5, 1, 0);

  ExpectIsometryNear(link_poses[0], link1_truth, "link1 pose is wrong");

  Eigen::Isometry3d link2_truth = Eigen::Isometry3d::Identity();
  link2_truth.linear() = Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d(0, 0, 1)).toRotationMatrix();
  link2_truth.translation() = Eigen::Vector3d(0.5, 1, 1);

  ExpectIsometryNear(link_poses[1], link2_truth, "link2 pose is wrong");
}

TEST_F(EigenForwardKinematicsPluginTreeTest, SimpleUrdfComputeJointTreeReversed)
{
  // One frame attached to link2 with identity origin
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

  auto link_names = std::vector<std::string>{"base_link", "link1", "link2"};
  auto subanal = AnalysisTree(anal, "link1", link_names);

  const auto & joints = subanal.get_joints();
  const auto & frames = subanal.get_frames();

  ExpectIsometryNear(joints[0].origin, Eigen::Isometry3d::Identity(),
    "dummy root origin is incorrect");
  ExpectIsometryNear(joints[1].origin, to_isometry(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0
  ), "joint 1 origin is incorrect");  //< Now identity, as original output has become root
  ExpectIsometryNear(joints[2].origin, to_isometry(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 1.0
  ), "joint 2 origin is incorrect");  //< Should be same as forward case

  EXPECT_EQ(frames["base_link"], 0) << "Frame order modified before sorting";
  EXPECT_EQ(frames["link1"], 1) << "Frame order modified before sorting";
  EXPECT_EQ(frames["link2"], 2) << "Frame order modified before sorting";

  EXPECT_EQ(joints[0].parent, 0) << "Dummy root is not a dummy root";
  ExpectIsometryNear(joints[0].origin, Eigen::Isometry3d::Identity(), "Dummy root has non-identity origin.");

  EXPECT_EQ(frames[0].parent, joints["joint1"]) << "base_link parent incorrect";
  EXPECT_EQ(frames[1].parent, 0) << "link1 parent incorrect, should be dummy root";
  EXPECT_EQ(frames[2].parent, joints["joint2"]) << "link2 parent incorrect";
  ExpectIsometryNear(frames[0].origin, anal.get_joints()[anal.get_joints()["joint1"]].origin.inverse(),
    "base_link origin is incorrect");
  ExpectIsometryNear(frames[1].origin, Eigen::Isometry3d::Identity(),
    "link1 origin is incorrect");
  ExpectIsometryNear(frames[2].origin, Eigen::Isometry3d::Identity(),
    "link2 origin is incorrect");

  auto tree = subanal.make_compute_joint_tree();

  EXPECT_EQ(tree.poses.size(), joint_states.size()) << "Wrong number of joints in the tree. Is there redundancy?";
  EXPECT_EQ(tree.get_root_relative_count(), 2) << "Wrong root relative joint count!";

  RCLCPP_INFO(node_->get_logger(), "Performing FK");

  const std::vector<std::string> & mapper_joint_names = {subanal.get_joints().names.begin() + 1, subanal.get_joints().names.end()};
  const auto joint_map = plugin->get_joint_map_builder().build(joint_names, mapper_joint_names);
  std::vector<double> joint_states_mapped(joint_map.output_count);
  joint_map.map(joint_states, joint_states_mapped);

  tree.update(joint_states_mapped);

  // Compare against truth
  RCLCPP_INFO(node_->get_logger(), "Testing against true poses");

  ExpectVectorNear(tree.get_axes()[0], Eigen::Vector3d(-1, 0, 0), "ComputeJointTree axis[0] is incorrect!");
  ExpectVectorNear(tree.get_axes()[1], Eigen::Vector3d(0, 0, 1), "ComputeJointTree axis[1] is incorrect!");
  EXPECT_EQ(tree.get_types()[0], arm_kinematics::JointType::PRISMATIC);
  EXPECT_EQ(tree.get_types()[1], arm_kinematics::JointType::REVOLUTE);
  ExpectIsometryNear(tree.get_origins()[0], to_isometry(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0
  ), "origin 0 is incorrect");  //< Now identity, as original output has become root
  ExpectIsometryNear(tree.get_origins()[1], to_isometry(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 1.0
  ), "origin 1 is incorrect");

  Eigen::Isometry3d j1_truth = Eigen::Isometry3d::Identity();
  j1_truth.translation() = Eigen::Vector3d(-0.5, 0, 0);

  ExpectIsometryNear(tree.poses[0], j1_truth, "joint 1 pose is wrong");

  Eigen::Isometry3d j2_truth = Eigen::Isometry3d::Identity();
  j2_truth.linear() = Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d(0, 0, 1)).toRotationMatrix();
  j2_truth.translation() = Eigen::Vector3d(0.0, 0.0, 1);

  ExpectIsometryNear(tree.poses[1], j2_truth, "joint 2 pose is wrong");
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

