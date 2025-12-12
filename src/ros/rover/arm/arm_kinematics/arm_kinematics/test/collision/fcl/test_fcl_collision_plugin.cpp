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

#include "arm_kinematics/collision/collider_definitions.hpp"
#include "arm_kinematics/collision/collision_manager.hpp"
#include "arm_kinematics/collision/discrete_collision_plugin.hpp"
#include "arm_kinematics/collision/fcl/fcl_collision_plugin.hpp"

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
using arm_kinematics::FclCollisionPlugin;

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

        <link name="link2"/>
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
    ASSERT_TRUE(model_.initString(robot_description_));

    node_ = std::make_shared<rclcpp::Node>("test_eigen_fk_mapper");
    logger_ = node_->get_logger();
    kinematics_params_ = std::make_shared<KinematicsParams>(*node_, robot_description_);

    fk_plugin_ = std::make_shared<EigenForwardKinematicsPlugin>();
    init_result_ = fk_plugin_->initialize(*node_, kinematics_params_);


    auto collision = std::make_shared<FclCollisionPlugin>();
    auto [colliders, frames, acm] = arm_kinematics::ColliderDefinitions(model_);

    auto [tree, order] = fk_plugin_->make_tree(joint_names_, "base_link", std::move(frames));
    init_result_ = init_result_ && collision->initialize(*node_, order.reorder(std::move(colliders)), std::move(acm));

    manager_ = arm_kinematics::CollisionManager(std::move(tree), std::move(collision));
  }

  void TearDown() override {
    node_.reset();
  }

  std::string robot_description_;
  urdf::Model model_;
  std::shared_ptr<rclcpp::Node> node_;
  rclcpp::Logger logger_ = rclcpp::get_logger("not_initialized");
  KinematicsParams::SharedPtr kinematics_params_;

  ForwardKinematicsPlugin::SharedPtr fk_plugin_;
  bool init_result_ = false;

  arm_kinematics::CollisionManager manager_{};

  std::vector<std::string> joint_names_ = {"j1", "j2"};
};

TEST_F(SimpleUrdfCollisionTests, SimpleCollisions)
{
  ASSERT_TRUE(init_result_) << "Failed to init either fk or collision plugin";

  manager_.update_poses({0,0});
  ASSERT_FALSE(manager_.collide()) << "Collision found when there should not be a collision!";

  manager_.update_poses({-2,-2});
  ASSERT_FALSE(manager_.collide()) << "Collision not found when there should be a collision!";

}


