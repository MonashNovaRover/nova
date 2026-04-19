//
// Created by Bailey Chessum on 17/11/2025.
//

#include <gtest/gtest.h>
#include <urdf_model/model.h>
#include <Eigen/Geometry>

#include <pluginlib/class_loader.hpp>
#include <rclcpp/rclcpp.hpp>

#include "arm_kinematics/forward/utilities/compute_joint_tree.hpp"
#include "arm_kinematics/forward/utilities/compute_frame_tree.hpp"
#include "arm_kinematics/common/robot_model.hpp"
#include "arm_kinematics/plugins/forward/default_forward_kinematics_plugin.hpp"
#include "arm_kinematics/joint_map/joint_map.hpp"
#include "arm_kinematics/joint_map/joint_map_builder.hpp"
#include "arm_kinematics/joint_map/state_interface_definition.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/reordered.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/collision/collision_manager.hpp"
#include "arm_kinematics/collision/collision_config.hpp"
#include "arm_kinematics/collision/collision_utilities.hpp"
#include "arm_kinematics/collision/discrete_collision_plugin.hpp"
#include "arm_kinematics/plugins/collision/fcl/fcl_collision_plugin.hpp"
#include "arm_kinematics/collision/collider_definitions.hpp"
#include "arm_kinematics/plugin_loader.hpp"

using arm_kinematics::ComputeFrameTree;
using arm_kinematics::ComputeJointTree;
using arm_kinematics::FrameDefinitions;
using arm_kinematics::JointMap;
using arm_kinematics::JointMapBuilder;
using arm_kinematics::ForwardKinematicsPlugin;
using arm_kinematics::DefaultForwardKinematicsPlugin;
using arm_kinematics::AnalysisTree;
using arm_kinematics::KinematicsParams;
using arm_kinematics::Reordered;
using arm_kinematics::FclCollisionPlugin;
using arm_kinematics::RobotModel;

namespace {

constexpr double EPSILON = 1e-7;

static void ExpectVectorNear(const Eigen::Vector3d & actual,
                             const Eigen::Vector3d & expected,
                             const char * message = "", double tol = EPSILON)
{
  EXPECT_NEAR(actual.x(), expected.x(), tol) << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix();
  EXPECT_NEAR(actual.y(), expected.y(), tol) << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix();
  EXPECT_NEAR(actual.z(), expected.z(), tol) << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix();
}

// Small helper for comparing isometries
static void ExpectIsometryNear(const Eigen::Isometry3d & actual,
                               const Eigen::Isometry3d & expected,
                               const char * message = "", double tol = EPSILON)
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

}

class SimpleUrdfCollisionTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    robot_description_ = R"(
      <robot name="test_robot">
        <link name="base_link"/>

        <link name="link1">
          <collision>
            <geometry>
              <sphere radius="0.5"/>
            </geometry>
          </collision>
        </link>

        <link name="link2">
          <collision>
            <geometry>
              <sphere radius="0.5"/>
            </geometry>
          </collision>
        </link>

        <joint name="j1" type="prismatic">
          <parent link="base_link"/>
          <child  link="link1"/>
          <origin xyz="0 2 0" rpy="0 0 0"/>
          <axis   xyz="0 1 0"/>
          <limit  lower="-1.0" upper="1.0" effort="10.0" velocity="10.0"/>
        </joint>

        <joint name="j2" type="prismatic">
          <parent link="link1"/>
          <child  link="link2"/>
          <origin xyz="0 0 2" rpy="0 0 0"/>
          <axis   xyz="0 0 1"/>
          <limit  lower="-3.14159" upper="3.14159" effort="10.0" velocity="10.0"/>
        </joint>
      </robot>
    )";
    ASSERT_TRUE(urdf::Model().initString(robot_description_));

    node_ = std::make_shared<rclcpp::Node>("test_fcl_collision_plugin");
    logger_ = node_->get_logger();
    robot_model_ = std::make_unique<RobotModel>(robot_description_);
    kinematics_params_ = std::make_shared<KinematicsParams>(*node_);

    fk_plugin_ = std::make_shared<DefaultForwardKinematicsPlugin>();
    init_result_ = fk_plugin_->initialize(*node_, *robot_model_, kinematics_params_);
    ASSERT_TRUE(init_result_);

    auto collision = std::make_shared<FclCollisionPlugin>();
    auto [colliders, frames, acm] = arm_kinematics::ColliderDefinitions(robot_model_->get_urdf_model());
    parent_link_names_ = frames.parent_link_names;

    // Build position-interface NamedStateInterfaceDefinitions for the joint inputs and call
    // the new make_tree convenience overload, which returns tl::expected<MakeTreeResult, ...>.
    static const arm_kinematics::InterfaceId k_position{"position"};
    std::vector<arm_kinematics::NamedStateInterfaceDefinition> named_inputs;
    named_inputs.reserve(joint_names_.size());
    for (const auto & name : joint_names_) {
      named_inputs.emplace_back(name, k_position);
    }

    auto make_tree_result = fk_plugin_->make_tree(
      arm_kinematics::span<const arm_kinematics::NamedStateInterfaceDefinition>(
        named_inputs.data(), named_inputs.size()),
      "base_link",
      frames);
    ASSERT_TRUE(make_tree_result.has_value())
      << "make_tree failed: " << make_tree_result.error().format();
    auto tree = std::move(make_tree_result.value().tree);
    const auto & order = make_tree_result.value().frame_order;
    parent_link_names_ = order.reorder(std::move(parent_link_names_));

    init_result_ = collision->initialize(*node_, order.reorder(std::move(colliders)), std::move(acm));
    ASSERT_TRUE(init_result_);

    tree_ = std::move(tree);
    collision_plugin_ = collision;
    manager_ = arm_kinematics::CollisionManager(tree_, collision_plugin_);
  }

  void TearDown() override {
    node_.reset();
  }

  std::string robot_description_;
  RobotModel::UniquePtr robot_model_;
  std::shared_ptr<rclcpp::Node> node_;
  rclcpp::Logger logger_ = rclcpp::get_logger("not_initialized");
  KinematicsParams::SharedPtr kinematics_params_;

  ForwardKinematicsPlugin::SharedPtr fk_plugin_;
  bool init_result_ = false;

  arm_kinematics::CollisionManager manager_{};
  ForwardKinematicsPlugin::Tree::SharedPtr tree_{};
  arm_kinematics::DiscreteCollisionPlugin::SharedPtr collision_plugin_{};
  std::vector<std::string> parent_link_names_{};

  std::vector<std::string> joint_names_ = {"j1", "j2"};
};

TEST_F(SimpleUrdfCollisionTests, SimpleCollisions)
{
  ASSERT_TRUE(init_result_) << "Failed to init either fk or collision plugin";

  manager_.update_poses({0,0});
  ASSERT_FALSE(manager_.collide()) << "Collision found when there should not be a collision!";

  manager_.update_poses({-2,-2});
  ASSERT_TRUE(manager_.collide()) << "Collision not found when there should be a collision!";
}

TEST_F(SimpleUrdfCollisionTests, CollideWithPairsReturnsDetectedColliderIndices)
{
  ASSERT_TRUE(init_result_) << "Failed to init either fk or collision plugin";

  manager_.update_poses({-2, -2});
  std::vector<std::pair<size_t, size_t>> colliding_pairs;
  ASSERT_TRUE(manager_.collide(colliding_pairs));
  ASSERT_FALSE(colliding_pairs.empty());
  EXPECT_EQ(colliding_pairs[0], std::make_pair(std::size_t{0}, std::size_t{1}));
}

TEST_F(SimpleUrdfCollisionTests, ProbeAndAllowCollisionsAllowsPairsAtConfiguredPose)
{
  ASSERT_TRUE(init_result_) << "Failed to init either fk or collision plugin";

  arm_kinematics::probe_and_allow_collisions(
    *collision_plugin_,
    *tree_,
    std::vector<double>{-2.0, -2.0});

  manager_.update_poses({-2, -2});
  std::vector<std::pair<size_t, size_t>> colliding_pairs;
  EXPECT_FALSE(manager_.collide(colliding_pairs));
  EXPECT_TRUE(colliding_pairs.empty());
  EXPECT_TRUE(collision_plugin_->get_allowed_collision_matrix().get(0, 1));
}

TEST_F(SimpleUrdfCollisionTests, MakeCollisionManagerWithConfigProbesDefaultPose)
{
  arm_kinematics::PluginLoader loader(*node_, robot_description_);
  std::shared_ptr<arm_kinematics::ForwardKinematicsPlugin> fk;
  try {
    fk = loader.make_fk("arm_kinematics/DefaultForwardKinematicsPlugin");
  } catch (const pluginlib::PluginlibException & e) {
    GTEST_SKIP() << "pluginlib unavailable in this environment: " << e.what();
  }
  ASSERT_TRUE(fk);

  arm_kinematics::CollisionConfig config;
  config.default_pose_overrides["j1"] = -2.0;
  config.default_pose_overrides["j2"] = -2.0;

  auto manager_result = arm_kinematics::make_collision_manager(loader, fk, joint_names_, config);
  ASSERT_TRUE(manager_result.has_value()) << manager_result.error().format();

  auto manager = std::move(manager_result.value());
  manager.update_poses({-2.0, -2.0});
  std::vector<std::pair<size_t, size_t>> colliding_pairs;
  EXPECT_FALSE(manager.collide(colliding_pairs));
  EXPECT_TRUE(colliding_pairs.empty());
}

TEST_F(SimpleUrdfCollisionTests, MakeCollisionManagerWithConfigAllowsPairsByLink)
{
  arm_kinematics::PluginLoader loader(*node_, robot_description_);
  std::shared_ptr<arm_kinematics::ForwardKinematicsPlugin> fk;
  try {
    fk = loader.make_fk("arm_kinematics/DefaultForwardKinematicsPlugin");
  } catch (const pluginlib::PluginlibException & e) {
    GTEST_SKIP() << "pluginlib unavailable in this environment: " << e.what();
  }
  ASSERT_TRUE(fk);

  arm_kinematics::CollisionConfig config;
  config.generate_from_default_pose = false;
  config.allowed_pairs.emplace_back("link1", "link2");

  auto manager_result = arm_kinematics::make_collision_manager(loader, fk, joint_names_, config);
  ASSERT_TRUE(manager_result.has_value()) << manager_result.error().format();

  auto manager = std::move(manager_result.value());
  manager.update_poses({-2.0, -2.0});
  std::vector<std::pair<size_t, size_t>> colliding_pairs;
  EXPECT_FALSE(manager.collide(colliding_pairs));
  EXPECT_TRUE(colliding_pairs.empty());
}
