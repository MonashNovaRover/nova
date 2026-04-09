//
// Created by Bailey Chessum on 9/4/26.
//
// `EigenForwardKinematicsPlugin::make_tree` end-to-end tests with synthetic URDFs.
//

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <Eigen/Geometry>
#include <rclcpp/rclcpp.hpp>
#include <urdf/model.h>

#include "arm_kinematics/common/kinematics_params.hpp"
#include "arm_kinematics/common/robot_model.hpp"
#include "arm_kinematics/forward/forward_kinematics_plugin.hpp"
#include "arm_kinematics/forward/frame_definitions.hpp"
#include "arm_kinematics/forward/utilities/analysis_tree.hpp"
#include "arm_kinematics/joint_map/joint_map_builder.hpp"
#include "arm_kinematics/joint_map/state_interface_definition.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/plugins/forward/eigen_forward_kinematics_plugin.hpp"
#include "arm_kinematics/utilities/aliases.hpp"
#include "arm_kinematics/utilities/order.hpp"
#include "arm_kinematics/utilities/span.hpp"

namespace arm_kinematics {

namespace {

constexpr double kEpsilon = 1e-7;

void ExpectVectorNear(
  const Eigen::Vector3f & actual,
  const Eigen::Vector3f & expected,
  const char * message = "",
  double tol = kEpsilon)
{
  EXPECT_NEAR(actual.x(), expected.x(), tol)
    << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix();
  EXPECT_NEAR(actual.y(), expected.y(), tol)
    << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix();
  EXPECT_NEAR(actual.z(), expected.z(), tol)
    << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix();
}

void ExpectIsometryNear(
  const Eigen::Isometry3f & actual,
  const Eigen::Isometry3f & expected,
  const char * message = "",
  double tol = kEpsilon)
{
  ExpectVectorNear(actual.translation(), expected.translation(), message, tol);
  EXPECT_TRUE(actual.linear().isApprox(expected.linear(), tol))
    << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix() << "\n";
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
  return Eigen::Isometry3f(m);
}

// Helper: build a vector of position-interface NamedStateInterfaceDefinitions from a list of
// joint names. The new make_tree convenience overload takes span<const NamedStateInterfaceDefinition>;
// callers most often want all-position inputs, which this helper produces.
std::vector<NamedStateInterfaceDefinition> position_inputs(
  const std::vector<std::string> & joint_names)
{
  static const InterfaceId k_position{"position"};
  std::vector<NamedStateInterfaceDefinition> result;
  result.reserve(joint_names.size());
  for (const auto & name : joint_names) {
    result.push_back(NamedStateInterfaceDefinition{name, k_position});
  }
  return result;
}

}  // namespace

// ===========================================================================
// SimpleUrdfTests fixture: 2-joint arm (prismatic + revolute)
// ===========================================================================

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
    ASSERT_TRUE(urdf::Model().initString(robot_description_));
    robot_model_ = std::make_unique<RobotModel>(robot_description_);

    node_ = std::make_shared<rclcpp::Node>("test_eigen_make_tree");
    kinematics_params_ = std::make_shared<KinematicsParams>(*node_);
  }

  void TearDown() override
  {
    node_.reset();
  }

  std::string robot_description_;
  RobotModel::UniquePtr robot_model_;
  std::shared_ptr<rclcpp::Node> node_;
  KinematicsParams::SharedPtr kinematics_params_;
};

TEST_F(SimpleUrdfTests, AnalysisTreeBuildsExpectedJointPosesFromUrdf)
{
  // This test exercises AnalysisTree internals (sort_joints, make_compute_joint_tree, pose
  // computation) — independent of the FK plugin's make_tree, but kept here because it's
  // load-bearing for the FK pipeline that follows.
  ForwardKinematicsPlugin::SharedPtr plugin = std::make_shared<EigenForwardKinematicsPlugin>();
  ASSERT_TRUE(plugin->initialize(*node_, *robot_model_, kinematics_params_));

  const float theta = static_cast<float>(M_PI / 2.0);
  const float d = 0.5F;
  std::vector<float> joint_states{d, theta};

  AnalysisTree anal(plugin->get_robot_model().get_urdf_model());

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

  anal.sort_joints();
  auto tree = anal.make_compute_joint_tree();

  ASSERT_EQ(tree.poses.size(), joint_states.size())
    << "Wrong number of joints in the tree. Is there redundancy?";
  ASSERT_EQ(tree.get_root_relative_count(), 1) << "Wrong root relative joint count!";

  tree.update(joint_states);

  Eigen::Isometry3f link1_truth = Eigen::Isometry3f::Identity();
  link1_truth.translation() = Eigen::Vector3f(0.5, 1, 0);
  ExpectIsometryNear(tree.poses[0], link1_truth, "link1 pose is wrong");

  Eigen::Isometry3f link2_truth = Eigen::Isometry3f::Identity();
  link2_truth.linear() =
    Eigen::AngleAxisf(M_PI / 2.0, Eigen::Vector3f(0, 0, 1)).toRotationMatrix();
  link2_truth.translation() = Eigen::Vector3f(0.5, 1, 1);
  ExpectIsometryNear(tree.poses[1], link2_truth, "link2 pose is wrong");
}

TEST_F(SimpleUrdfTests, MakeTreeProducesCorrectFrameTransforms)
{
  // End-to-end FK test using the new make_tree API: build the tree, run position_fk on
  // joint inputs, verify the resulting frame poses against ground truth.
  FrameDefinitions frames = {std::vector<std::string>{"link1", "link2"}};
  const std::size_t output_count = frames.origins.size();

  EigenForwardKinematicsPlugin::SharedPtr plugin =
    std::make_shared<EigenForwardKinematicsPlugin>();
  ASSERT_TRUE(plugin->initialize(*node_, *robot_model_, kinematics_params_));

  const float theta = static_cast<float>(M_PI / 2.0);
  const float d = 0.5F;
  std::vector<float> joint_states{d, theta};

  // Build the tree using the named convenience overload of make_tree.
  const auto named_inputs = position_inputs({"joint1", "joint2"});
  const auto result = plugin->make_tree(
    span<const NamedStateInterfaceDefinition>(named_inputs.data(), named_inputs.size()),
    std::string("base_link"),
    frames);

  ASSERT_TRUE(result.has_value()) << "make_tree failed: " << result.error().message;
  auto tree_ = std::move(result.value().tree);
  ASSERT_TRUE(tree_) << "make_tree returned null tree";

  auto * tree = dynamic_cast<EigenForwardKinematicsPlugin::TreeImpl *>(tree_.get());
  ASSERT_TRUE(tree) << "Failed to cast to TreeImpl";

  EXPECT_EQ(tree->get_joint_map().input_count(), 2u);
  EXPECT_EQ(tree->get_joint_map().output_count(), 2u);
  EXPECT_TRUE(tree->get_joint_map().valid());
  EXPECT_EQ(tree->get_mapped_joint_states().size(), 2u);

  Isometry3fVector link_poses(output_count, Eigen::Isometry3f::Identity());
  tree->position_fk(joint_states, link_poses);

  Eigen::Isometry3f link1_truth = Eigen::Isometry3f::Identity();
  link1_truth.translation() = Eigen::Vector3f(0.5, 1, 0);
  ExpectIsometryNear(link_poses[0], link1_truth, "link1 pose is wrong");

  Eigen::Isometry3f link2_truth = Eigen::Isometry3f::Identity();
  link2_truth.linear() =
    Eigen::AngleAxisf(M_PI / 2.0, Eigen::Vector3f(0, 0, 1)).toRotationMatrix();
  link2_truth.translation() = Eigen::Vector3f(0.5, 1, 1);
  ExpectIsometryNear(link_poses[1], link2_truth, "link2 pose is wrong");
}

TEST_F(SimpleUrdfTests, AnalysisTreeReversedSubtreeReorientsJoints)
{
  // The reversed-subtree case: build a subtree rooted at a non-root link, verify the joint
  // origins flip correctly. This is an AnalysisTree internals test; the FK plugin doesn't
  // directly drive it.
  ForwardKinematicsPlugin::SharedPtr plugin = std::make_shared<EigenForwardKinematicsPlugin>();
  ASSERT_TRUE(plugin->initialize(*node_, *robot_model_, kinematics_params_));

  AnalysisTree anal(plugin->get_robot_model().get_urdf_model());

  const std::vector<std::string> link_names{"base_link", "link1", "link2"};
  AnalysisTree subanal(anal, "link1", link_names);

  const auto & joints = subanal.get_joints();
  ExpectIsometryNear(joints[0].origin, Eigen::Isometry3f::Identity(),
    "dummy root origin is incorrect");
  ExpectIsometryNear(joints[1].origin, to_isometry(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0
  ), "joint 1 origin is incorrect (reversed)");
  ExpectIsometryNear(joints[2].origin, to_isometry(
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 1.0
  ), "joint 2 origin is incorrect (reversed)");

  auto tree = subanal.make_compute_joint_tree();
  EXPECT_EQ(tree.poses.size(), 2u) << "Wrong number of joints in the reversed tree";
  EXPECT_EQ(tree.get_root_relative_count(), 2) << "Wrong root relative joint count";
}

// ===========================================================================
// FixedJointUrdfTest fixture: a complex multi-branch URDF with mixed
// fixed + prismatic joints. Tests FK from various base links and stress.
// ===========================================================================

class FixedJointUrdfTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // x is a main branch dynamic joint
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
      </robot>
    )";
    ASSERT_TRUE(urdf::Model().initString(robot_description_));
    robot_model_ = std::make_unique<RobotModel>(robot_description_);

    node_ = std::make_shared<rclcpp::Node>("test_eigen_make_tree_fixed_joint");
    kinematics_params_ = std::make_shared<KinematicsParams>(*node_);

    plugin_ = std::make_shared<EigenForwardKinematicsPlugin>();
    init_result_ = plugin_->initialize(*node_, *robot_model_, kinematics_params_);
  }

  void TearDown() override
  {
    node_.reset();
  }

  std::string robot_description_;
  RobotModel::UniquePtr robot_model_;
  std::shared_ptr<rclcpp::Node> node_;
  KinematicsParams::SharedPtr kinematics_params_;

  ForwardKinematicsPlugin::SharedPtr plugin_;
  bool init_result_ = false;

  const std::vector<std::string> joint_names_{"zx", "zxzx", "zy", "x", "y", "zxzy", "zxy"};
  const std::vector<std::string> frame_names_{
    "base", "z", "zx", "zxz", "zxzx", "zxzxz", "zy", "zyz",
    "x", "y", "zz", "zzz", "zxzy", "zxy",
  };
  const Vector3fVector expected_frame_poses_{
    {0,0,0}, {0,0,1}, {1,0,1}, {1,0,2}, {2,0,2}, {2,0,3},
    {0,1,1}, {0,1,2}, {1,0,0}, {0,1,0}, {0,0,2}, {0,0,3},
    {1,1,2}, {1,1,1}
  };
};

TEST_F(FixedJointUrdfTest, ForwardFromRoot)
{
  ASSERT_TRUE(init_result_) << "Failed to init plugin";

  // Empty inputs — the FK tree has no dynamic joints to drive (we only test fixed-joint
  // propagation from the base link).
  const std::vector<NamedStateInterfaceDefinition> empty_inputs;
  const auto result = plugin_->make_tree(
    span<const NamedStateInterfaceDefinition>(empty_inputs.data(), empty_inputs.size()),
    "base",
    {frame_names_});
  ASSERT_TRUE(result.has_value()) << "make_tree failed: " << result.error().message;
  auto tree = std::move(result.value().tree);
  const auto & order = result.value().frame_order;
  ASSERT_TRUE(tree) << "Failed to make tree";

  const auto names = Reordered{frame_names_, order};
  const auto expected = Reordered{expected_frame_poses_, order};

  Isometry3fVector actual(expected.size());
  tree->position_fk({}, actual);

  for (std::size_t i = 0; i < actual.size(); ++i) {
    ExpectVectorNear(actual[i].translation(), expected[i], names[i].c_str());
  }
}

TEST_F(FixedJointUrdfTest, ForwardFromRootWithActuations)
{
  ASSERT_TRUE(init_result_) << "Failed to init plugin";

  const auto named_inputs = position_inputs(joint_names_);
  const auto result = plugin_->make_tree(
    span<const NamedStateInterfaceDefinition>(named_inputs.data(), named_inputs.size()),
    "base",
    {frame_names_});
  ASSERT_TRUE(result.has_value()) << "make_tree failed: " << result.error().message;
  auto tree = std::move(result.value().tree);
  const auto & order = result.value().frame_order;
  ASSERT_TRUE(tree) << "Failed to make tree";

  const auto names = Reordered{frame_names_, order};
  const auto expected = Reordered{expected_frame_poses_, order};

  // Set joint states to cancel out all axes but the z axis.
  const std::vector<float> joint_states(joint_names_.size(), -1.0F);

  Isometry3fVector actual(expected.size());
  tree->position_fk(joint_states, actual);

  for (std::size_t i = 0; i < actual.size(); ++i) {
    ExpectVectorNear(
      actual[i].translation(),
      Eigen::Vector3f(0, 0, expected[i].z()),
      names[i].c_str());
  }
}

TEST_F(FixedJointUrdfTest, BackwardFromAll)
{
  ASSERT_TRUE(init_result_) << "Failed to init plugin";

  const std::vector<NamedStateInterfaceDefinition> empty_inputs;

  for (std::size_t i = 0; i < frame_names_.size(); ++i) {
    const auto & base_name = frame_names_[i];

    const auto result = plugin_->make_tree(
      span<const NamedStateInterfaceDefinition>(empty_inputs.data(), empty_inputs.size()),
      base_name,
      {frame_names_});
    ASSERT_TRUE(result.has_value())
      << "make_tree failed from base \"" << base_name << "\": " << result.error().message;
    auto tree = std::move(result.value().tree);
    const auto & order = result.value().frame_order;
    ASSERT_TRUE(tree) << "Failed to make tree from base \"" << base_name << "\"";

    const auto names = Reordered{frame_names_, order};
    const auto expected = Reordered{expected_frame_poses_, order};

    Isometry3fVector actual(expected.size());
    tree->position_fk({}, actual);

    const auto base = expected_frame_poses_[i];

    for (std::size_t j = 0; j < actual.size(); ++j) {
      EXPECT_EQ(actual[j].translation(), expected[j] - base)
        << "Incorrect pose from " << base_name << " to " << names[j];
    }
  }
}

TEST_F(FixedJointUrdfTest, StressTest)
{
  ASSERT_TRUE(init_result_) << "Failed to init plugin";

  using Clock = std::chrono::steady_clock;
  std::size_t us_total = 0;
  constexpr std::size_t kCalculationsPerIteration = 131072;

  const std::vector<NamedStateInterfaceDefinition> empty_inputs;

  for (std::size_t i = 0; i < frame_names_.size(); ++i) {
    const auto & base_name = frame_names_[i];

    const auto tree_start = Clock::now();
    const auto result = plugin_->make_tree(
      span<const NamedStateInterfaceDefinition>(empty_inputs.data(), empty_inputs.size()),
      base_name,
      {frame_names_});
    const auto tree_end = Clock::now();
    const auto tree_us = std::chrono::duration_cast<std::chrono::microseconds>(
      tree_end - tree_start).count();
    std::cout << "Tree construction took " << tree_us << " µs\n";

    ASSERT_TRUE(result.has_value())
      << "make_tree failed from base \"" << base_name << "\": " << result.error().message;
    auto tree = std::move(result.value().tree);
    ASSERT_TRUE(tree) << "Failed to make tree from base \"" << base_name << "\"";

    const auto & order = result.value().frame_order;
    const auto expected = Reordered{expected_frame_poses_, order};

    Isometry3fVector actual(expected.size());

    std::cout << "Stress testing from base \"" << base_name << "\"\n";
    const auto start = Clock::now();
    for (std::size_t k = 0; k < kCalculationsPerIteration; ++k) {
      tree->position_fk({}, actual);
    }
    const auto end = Clock::now();
    const auto us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Took " << us << " µs\n\n";
    us_total += us;
  }

  const std::size_t total_count = kCalculationsPerIteration * frame_names_.size();
  std::cout << "Average of "
            << static_cast<long double>(us_total) / static_cast<long double>(total_count)
            << " µs per FK calculation\n";
  std::cout << "Average of "
            << 1e6 * static_cast<long double>(total_count) / static_cast<long double>(us_total)
            << " hz\n\n";
}

}  // namespace arm_kinematics
