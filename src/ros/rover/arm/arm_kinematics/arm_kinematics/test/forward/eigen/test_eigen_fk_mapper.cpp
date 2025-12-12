//
// Created by Bailey Chessum on 17/11/2025.
//

#include <gtest/gtest.h>
#include <urdf_model/model.h>
#include <Eigen/Geometry>

#include <arm_kinematics/forward/utilities/compute_joint_tree.hpp>
#include <arm_kinematics/forward/utilities/compute_frame_tree.hpp>
#include <arm_kinematics/forward/utilities/eigen_forward_kinematics_plugin.hpp>
#include <arm_kinematics/joint_map/joint_map.hpp>
#include <arm_kinematics/joint_map/joint_map_builder.hpp>
#include <rclcpp/rclcpp.hpp>
#include <arm_kinematics/utilities/reordered.hpp>
#include <chrono>



using arm_kinematics::ComputeFrameTree;
using arm_kinematics::ComputeJointTree;
using arm_kinematics::FrameDefinitions;
using arm_kinematics::JointMap;
using arm_kinematics::JointMapBuilder;
using arm_kinematics::ForwardKinematicsPlugin;
using arm_kinematics::EigenForwardKinematicsPlugin;
using arm_kinematics::AnalysisTree;
using arm_kinematics::KinematicsParams;
using arm_kinematics::Reordered;

static void ExpectVectorNear(const Eigen::Vector3f & actual,
                             const Eigen::Vector3f & expected,
                             const char * message = "", double tol = 1e-10)
{
  EXPECT_NEAR(actual.x(), expected.x(), tol) << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix();
  EXPECT_NEAR(actual.y(), expected.y(), tol) << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix();
  EXPECT_NEAR(actual.z(), expected.z(), tol) << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix();
}

// Small helper for comparing isometries
static void ExpectIsometryNear(const Eigen::Isometry3f & actual,
                               const Eigen::Isometry3f & expected,
                               const char * message = "", double tol = 1e-10)
{
  ExpectVectorNear(actual.translation(), expected.translation(), message, tol);
  // EXPECT_NEAR(actual.translation().x(), expected.translation().x(), tol) << actual.matrix() << " \n vs \n" << expected.matrix();
  // EXPECT_NEAR(actual.translation().y(), expected.translation().y(), tol) << actual.matrix() << "\n vs \n" << expected.matrix();
  // EXPECT_NEAR(actual.translation().z(), expected.translation().z(), tol) << actual.matrix() << "\n vs \n" << expected.matrix();
  EXPECT_TRUE(actual.linear().isApprox(expected.linear(), tol)) << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix() << "\n";
}

Eigen::Isometry3f to_isometry(
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

class SimpleUrdfTests : public ::testing::Test
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
    kinematics_params_ = std::make_shared<KinematicsParams>(*node_, robot_description_);
  }

  void TearDown() override {
    node_.reset();
  }

  std::string robot_description_;
  urdf::Model model_;
  std::shared_ptr<rclcpp::Node> node_;
  rclcpp::Logger logger_ = rclcpp::get_logger("not_initialized");
  KinematicsParams::SharedPtr kinematics_params_;
};

TEST_F(SimpleUrdfTests, SimpleUrdfComputeJointTree)
{
  // One frame attached to link2 with identity origin
  std::vector<std::string> joint_names{"joint1", "joint2"};

  // Build mapper + joint names from URDF
  RCLCPP_INFO(node_->get_logger(), "Creating plugin");
  ForwardKinematicsPlugin::SharedPtr plugin = std::make_shared<EigenForwardKinematicsPlugin>();
  RCLCPP_INFO(node_->get_logger(), "Initializing plugin");
  auto init_result = plugin->initialize(*node_, kinematics_params_);

  ASSERT_TRUE(init_result) << "Failed to initialize plugin";

  const float theta = M_PI / 2.0;
  const float d     = 0.5;
  std::vector<float> joint_states{ d, theta };

  RCLCPP_INFO(node_->get_logger(), "Creating FK Tree");
  auto anal = AnalysisTree(plugin->get_urdf_model());

  ExpectIsometryNear(anal.get_joints()[0].origin, Eigen::Isometry3f::Identity(),
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

  ExpectIsometryNear(anal.get_joints()[0].origin, Eigen::Isometry3f::Identity(),
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

  ExpectVectorNear(tree.get_axes()[0], Eigen::Vector3f(1, 0, 0), "ComputeJointTree axis[0] is incorrect!");
  ExpectVectorNear(tree.get_axes()[1], Eigen::Vector3f(0, 0, 1), "ComputeJointTree axis[1] is incorrect!");
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

  Eigen::Isometry3f link1_truth = Eigen::Isometry3f::Identity();
  link1_truth.translation() = Eigen::Vector3f(0.5, 1, 0);

  ExpectIsometryNear(tree.poses[0], link1_truth, "link1 pose is wrong");

  Eigen::Isometry3f link2_truth = Eigen::Isometry3f::Identity();
  link2_truth.linear() = Eigen::AngleAxisf(M_PI / 2.0, Eigen::Vector3f(0, 0, 1)).toRotationMatrix();
  link2_truth.translation() = Eigen::Vector3f(0.5, 1, 1);

  ExpectIsometryNear(tree.poses[1], link2_truth, "link2 pose is wrong");
}

TEST_F(SimpleUrdfTests, SimpleUrdfEigenFKPluginTree)
{
  // One frame attached to link2 with identity origin
  FrameDefinitions frames = {std::vector<std::string>{"link1", "link2"}};
  const size_t output_count = frames.origins.size();
  std::vector<std::string> joint_names{"joint1", "joint2"};

  // Build mapper + joint names from URDF
  RCLCPP_INFO(node_->get_logger(), "Creating plugin");
  EigenForwardKinematicsPlugin::SharedPtr plugin = std::make_shared<EigenForwardKinematicsPlugin>();
  RCLCPP_INFO(node_->get_logger(), "Initializing plugin");
  auto init_result = plugin->initialize(*node_, kinematics_params_);

  ASSERT_TRUE(init_result) << "Failed to initialize plugin";

  const double theta = M_PI / 2.0;
  const double d     = 0.5;
  std::vector<double> joint_states{ d, theta };

  auto weird_joint_map = plugin->get_joint_map_builder().build({"joint1", "joint2"}, {"joint1", "joint2", "joint1", "joint 1", "joint 2"});
  std::vector<float> mapped_joint_states_weird(weird_joint_map.output_count);
  weird_joint_map.map(joint_states, mapped_joint_states_weird);

  RCLCPP_INFO(node_->get_logger(), "Creating FK Tree");
  auto [tree_, order] = plugin->make_tree(joint_names, std::string("base_link"), frames);

  ASSERT_TRUE(order.size() > 0) << "Failed to create tree with order";

  EigenForwardKinematicsPlugin::TreeImpl::SharedPtr tree =
    std::dynamic_pointer_cast<EigenForwardKinematicsPlugin::TreeImpl>(tree_);
  ASSERT_TRUE(tree) << "Failed to create tree";

  std::vector<float> mapped_joint_states(tree->get_joint_map().output_count);
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
  arm_kinematics::Isometry3fVector link_poses(output_count, Eigen::Isometry3f::Identity());
  RCLCPP_INFO(node_->get_logger(), "Performing FK");

  tree->position_fk(joint_states, link_poses);

  EXPECT_NEAR(tree->get_mapped_joint_states()[0], joint_states[0], 1e-10) << "joint 0 value mapped incorrectly";
  EXPECT_NEAR(tree->get_mapped_joint_states()[1], joint_states[1], 1e-10) << "joint 1 value mapped incorrectly";


  // Compare against truth
  RCLCPP_INFO(node_->get_logger(), "Testing against true poses");

  Eigen::Isometry3f link1_truth = Eigen::Isometry3f::Identity();
  link1_truth.translation() = Eigen::Vector3f(0.5, 1, 0);

  ExpectIsometryNear(link_poses[0], link1_truth, "link1 pose is wrong");

  Eigen::Isometry3f link2_truth = Eigen::Isometry3f::Identity();
  link2_truth.linear() = Eigen::AngleAxisf(M_PI / 2.0, Eigen::Vector3f(0, 0, 1)).toRotationMatrix();
  link2_truth.translation() = Eigen::Vector3f(0.5, 1, 1);

  ExpectIsometryNear(link_poses[1], link2_truth, "link2 pose is wrong");
}

TEST_F(SimpleUrdfTests, SimpleUrdfComputeJointTreeReversed)
{
  // One frame attached to link2 with identity origin
  std::vector<std::string> joint_names{"joint1", "joint2"};

  // Build mapper + joint names from URDF
  RCLCPP_INFO(node_->get_logger(), "Creating plugin");
  ForwardKinematicsPlugin::SharedPtr plugin = std::make_shared<EigenForwardKinematicsPlugin>();
  RCLCPP_INFO(node_->get_logger(), "Initializing plugin");
  auto init_result = plugin->initialize(*node_, kinematics_params_);

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

  ExpectIsometryNear(joints[0].origin, Eigen::Isometry3f::Identity(),
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
  ExpectIsometryNear(joints[0].origin, Eigen::Isometry3f::Identity(), "Dummy root has non-identity origin.");

  EXPECT_EQ(frames[0].parent, joints["joint1"]) << "base_link parent incorrect";
  EXPECT_EQ(frames[1].parent, 0) << "link1 parent incorrect, should be dummy root";
  EXPECT_EQ(frames[2].parent, joints["joint2"]) << "link2 parent incorrect";
  ExpectIsometryNear(frames[0].origin, anal.get_joints()[anal.get_joints()["joint1"]].origin.inverse(),
    "base_link origin is incorrect");
  ExpectIsometryNear(frames[1].origin, Eigen::Isometry3f::Identity(),
    "link1 origin is incorrect");
  ExpectIsometryNear(frames[2].origin, Eigen::Isometry3f::Identity(),
    "link2 origin is incorrect");

  auto tree = subanal.make_compute_joint_tree();

  EXPECT_EQ(tree.poses.size(), joint_states.size()) << "Wrong number of joints in the tree. Is there redundancy?";
  EXPECT_EQ(tree.get_root_relative_count(), 2) << "Wrong root relative joint count!";

  RCLCPP_INFO(node_->get_logger(), "Performing FK");

  const std::vector<std::string> & mapper_joint_names = {subanal.get_joints().names.begin() + 1, subanal.get_joints().names.end()};
  const auto joint_map = plugin->get_joint_map_builder().build(joint_names, mapper_joint_names);
  std::vector<float> joint_states_mapped(joint_map.output_count);
  joint_map.map(joint_states, joint_states_mapped);

  tree.update(joint_states_mapped);

  // Compare against truth
  RCLCPP_INFO(node_->get_logger(), "Testing against true poses");

  ExpectVectorNear(tree.get_axes()[0], Eigen::Vector3f(-1, 0, 0), "ComputeJointTree axis[0] is incorrect!");
  ExpectVectorNear(tree.get_axes()[1], Eigen::Vector3f(0, 0, 1), "ComputeJointTree axis[1] is incorrect!");
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

  Eigen::Isometry3f j1_truth = Eigen::Isometry3f::Identity();
  j1_truth.translation() = Eigen::Vector3f(-0.5, 0, 0);

  ExpectIsometryNear(tree.poses[0], j1_truth, "joint 1 pose is wrong");

  Eigen::Isometry3f j2_truth = Eigen::Isometry3f::Identity();
  j2_truth.linear() = Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3f(0, 0, 1)).toRotationMatrix();
  j2_truth.translation() = Eigen::Vector3f(0.0, 0.0, 1);

  ExpectIsometryNear(tree.poses[1], j2_truth, "joint 2 pose is wrong");
}

class FixedJointUrdfTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // x is a main branch is a dynamic joint
    // y is a dynamic joint
    // z is a fixed joint
    robot_description_ = R"(
      <robot name="test_robot">
        <link name="base"/>

        <link name="z"/>
        <joint name="z" type="fixed">
          <parent link="base"/>
          <child  link="z"/>
          <origin xyz="0 0 1" rpy="0 0 0"/>
        </joint>

        <link name="zx"/>
        <joint name="zx" type="prismatic">
          <parent link="z"/>
          <child  link="zx"/>
          <origin xyz="1 0 0" rpy="0 0 0"/>
          <axis   xyz="1 0 0"/>
          <limit  lower="-1.0" upper="1.0" effort="10.0" velocity="10.0"/>
        </joint>

        <link name="zxz"/>
        <joint name="zxz" type="fixed">
          <parent link="zx"/>
          <child  link="zxz"/>
          <origin xyz="0 0 1" rpy="0 0 0"/>
        </joint>

        <link name="zxzx"/>
        <joint name="zxzx" type="prismatic">
          <parent link="zxz"/>
          <child  link="zxzx"/>
          <origin xyz="1 0 0" rpy="0 0 0"/>
          <axis   xyz="1 0 0"/>
          <limit  lower="-1.0" upper="1.0" effort="10.0" velocity="10.0"/>
        </joint>

        <link name="zxzxz"/>
        <joint name="zxzxz" type="fixed">
          <parent link="zxzx"/>
          <child  link="zxzxz"/>
          <origin xyz="0 0 1" rpy="0 0 0"/>
        </joint>

        <link name="zy"/>
        <joint name="zy" type="prismatic">
          <parent link="z"/>
          <child  link="zy"/>
          <origin xyz="0 1 0" rpy="0 0 0"/>
          <axis   xyz="0 1 0"/>
          <limit  lower="-1.0" upper="1.0" effort="10.0" velocity="10.0"/>
        </joint>

        <link name="zyz"/>
        <joint name="zyz" type="fixed">
          <parent link="zy"/>
          <child  link="zyz"/>
          <origin xyz="0 0 1" rpy="0 0 0"/>
        </joint>

        <link name="x"/>
        <joint name="x" type="prismatic">
          <parent link="base"/>
          <child  link="x"/>
          <origin xyz="1 0 0" rpy="0 0 0"/>
          <axis   xyz="1 0 0"/>
          <limit  lower="-1.0" upper="1.0" effort="10.0" velocity="10.0"/>
        </joint>

        <link name="y"/>
        <joint name="y" type="prismatic">
          <parent link="base"/>
          <child  link="y"/>
          <origin xyz="0 1 0" rpy="0 0 0"/>
          <axis   xyz="0 1 0"/>
          <limit  lower="-1.0" upper="1.0" effort="10.0" velocity="10.0"/>
        </joint>

        <link name="zz"/>
        <joint name="zz" type="fixed">
          <parent link="z"/>
          <child  link="zz"/>
          <origin xyz="0 0 1" rpy="0 0 0"/>
        </joint>

        <link name="zzz"/>
        <joint name="zzz" type="fixed">
          <parent link="zz"/>
          <child  link="zzz"/>
          <origin xyz="0 0 1" rpy="0 0 0"/>
        </joint>

        <link name="zxzy"/>
        <joint name="zxzy" type="prismatic">
          <parent link="zxz"/>
          <child  link="zxzy"/>
          <origin xyz="0 1 0" rpy="0 0 0"/>
          <axis   xyz="0 1 0"/>
          <limit  lower="-1.0" upper="1.0" effort="10.0" velocity="10.0"/>
        </joint>

        <link name="zxy"/>
        <joint name="zxy" type="prismatic">
          <parent link="zx"/>
          <child  link="zxy"/>
          <origin xyz="0 1 0" rpy="0 0 0"/>
          <axis   xyz="0 1 0"/>
          <limit  lower="-1.0" upper="1.0" effort="10.0" velocity="10.0"/>
        </joint>

        <ros2_control>
        </ros2_control>
      </robot>
    )";
    ASSERT_TRUE(model_.initString(robot_description_));

    node_ = std::make_shared<rclcpp::Node>("test_eigen_fk_mapper");
    logger_ = node_->get_logger();
    kinematics_params_ = std::make_shared<KinematicsParams>(*node_, robot_description_);

    plugin_ = std::make_shared<EigenForwardKinematicsPlugin>();
    init_result_ = plugin_->initialize(*node_, kinematics_params_);
  }

  void TearDown() override {
    node_.reset();
  }

  std::string robot_description_;
  urdf::Model model_;
  std::shared_ptr<rclcpp::Node> node_;
  rclcpp::Logger logger_ = rclcpp::get_logger("not_initialized");
  KinematicsParams::SharedPtr kinematics_params_;

  ForwardKinematicsPlugin::SharedPtr plugin_;
  bool init_result_ = false;

  const std::vector<std::string> joint_names_{"zx", "zxzx", "zy", "x", "y", "zxzy", "zxy"};
  const std::vector<std::string> frame_names_{
    "base",
    "z",
    "zx",
    "zxz",
    "zxzx",
    "zxzxz",
    "zy",
    "zyz",
    "x",
    "y",
    "zz",
    "zzz",
    "zxzy",
    "zxy",
  };
  const arm_kinematics::Vector3fVector expected_frame_poses_{
    {0,0,0},
    {0,0,1},
    {1,0,1},
    {1,0,2},
    {2,0,2},
    {2,0,3},
    {0,1,1},
    {0,1,2},
    {1,0,0},
    {0,1,0},
    {0,0,2},
    {0,0,3},
    {1,1,2},
    {1,1,1}
  };
};

TEST_F(FixedJointUrdfTest, ForwardFromRoot) {
  ASSERT_TRUE(init_result_) << "Failed to init plugin";

  auto [tree, order] = plugin_->make_tree({}, "base", {frame_names_});
  ASSERT_TRUE(tree) << "Failed to make tree";

  const auto names = Reordered{frame_names_, order};
  const auto expected = Reordered{expected_frame_poses_, order};

  // Do FK
  auto actual = arm_kinematics::Isometry3fVector(expected.size());
  tree->position_fk({}, actual);

  for (size_t i = 0; i < actual.size(); ++i) {
    ExpectVectorNear(actual[i].translation(), expected[i], names[i].c_str());
  }
}

TEST_F(FixedJointUrdfTest, ForwardFromRootWithActuations) {
  ASSERT_TRUE(init_result_) << "Failed to init plugin";

  auto [tree, order] = plugin_->make_tree(joint_names_, "base", {frame_names_});
  ASSERT_TRUE(tree) << "Failed to make tree";

  const auto names = Reordered{frame_names_, order};
  const auto expected = Reordered{expected_frame_poses_, order};

  // Set joint states to cancel out all axes but the z axis
  const std::vector<double> joint_states(joint_names_.size(), -1.0);

  // Do FK
  auto actual = arm_kinematics::Isometry3fVector(expected.size());
  tree->position_fk(joint_states, actual);

  for (size_t i = 0; i < actual.size(); ++i) {
    ExpectVectorNear(actual[i].translation(), Eigen::Vector3f(0,0,expected[i].z()), names[i].c_str());
  }
}

TEST_F(FixedJointUrdfTest, BackwardFromAll) {
  ASSERT_TRUE(init_result_) << "Failed to init plugin";

  for (size_t i = 0; i < frame_names_.size(); ++i) {
    const auto & base_name = frame_names_[i];

    auto [tree, order] = plugin_->make_tree(
      {}, base_name, {frame_names_});
    ASSERT_TRUE(tree) << "Failed to make tree from root \"" << joint_names_[i] << "\"";

    const auto names = Reordered{frame_names_, order};
    const auto expected = Reordered{expected_frame_poses_, order};

    // Do FK
    auto actual = arm_kinematics::Isometry3fVector(expected.size());
    tree->position_fk({}, actual);

    const auto base = expected_frame_poses_[i];

    for (size_t j = 0; j < actual.size(); ++j) {
      if (actual[j].translation() == expected[j] - base)
        std::cout << "\033[0;32m[PASS] " << base_name << " -> " << names[j] << " \033[0m\n";
      else
        std::cerr << "\033[0;31m[FAIL] " << base_name << " -> " << names[j] << " \033[0m\n";

      EXPECT_EQ(actual[j].translation(), expected[j] - base) << "Incorrect pose from " << base_name << " to " << names[j];
    }
  }
}

using Clock = std::chrono::steady_clock;

TEST_F(FixedJointUrdfTest, StressTest) {
  ASSERT_TRUE(init_result_) << "Failed to init plugin";

  size_t us_total = 0;

  for (size_t i = 0; i < frame_names_.size(); ++i) {
    const auto & base_name = frame_names_[i];

    auto tree_start = Clock::now();
    auto [tree, order] = plugin_->make_tree(
      {}, base_name, {frame_names_});
    auto tree_end   = Clock::now();
    auto tree_dur   = tree_end - tree_start;
    auto tree_us = std::chrono::duration_cast<std::chrono::microseconds>(tree_dur).count();
    std::cout << "Tree construction took " << tree_us << " µs\n\n";

    ASSERT_TRUE(tree) << "Failed to make tree from root \"" << joint_names_[i] << "\"";

    const auto names = Reordered{frame_names_, order};
    const auto expected = Reordered{expected_frame_poses_, order};

    // Do FK
    auto actual = arm_kinematics::Isometry3fVector(expected.size());

    std::cout << "Stress testing from root \"" << base_name << "\"\n";

    auto start = Clock::now();
    for (size_t k = 0; k < 10000000; ++k) {
      tree->position_fk({}, actual);
    }
    auto end   = Clock::now();
    auto dur   = end - start;

    auto us = std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
    std::cout << "Took " << us << " µs\n\n";
    us_total += us;
  }

  size_t total_count = 10000000 * frame_names_.size();

  std::cout << "Average of " << static_cast<long double>(us_total) / static_cast<long double>(total_count) << " µs per FK calculation\n";
  std::cout << "Average of " << 1e9 * static_cast<long double>(total_count) / static_cast<long double>(us_total) << " hz\n\n";
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  rclcpp::InitOptions options;
  options.auto_initialize_logging(false);
  options.set_domain_id(222);

  rclcpp::init(argc, argv, options);

  int ret = RUN_ALL_TESTS();

  rclcpp::shutdown();
  return ret;
}

