//
// Created by Bailey Chessum on 9/4/26.
//
// URDF-driven import tests for `TransmissionAnalysis`.
//

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include <urdf/model.h>

#include "arm_kinematics/common/robot_model.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_analysis_import.hpp"

namespace arm_kinematics {

namespace {

constexpr float kEpsilon = 1.0e-5F;

}  // namespace

// ===========================================================================
// MimicUrdfTests fixture
// ===========================================================================

class MimicUrdfTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    robot_description_ = R"(
      <robot name="mimic_robot">
        <link name="base_link"/>
        <link name="driver_link"/>
        <link name="follower_link"/>

        <joint name="driver_joint" type="revolute">
          <parent link="base_link"/>
          <child  link="driver_link"/>
          <origin xyz="0 0 0" rpy="0 0 0"/>
          <axis   xyz="0 0 1"/>
          <limit  lower="-3.14159" upper="3.14159" effort="10.0" velocity="10.0"/>
        </joint>

        <joint name="follower_joint" type="revolute">
          <parent link="driver_link"/>
          <child  link="follower_link"/>
          <origin xyz="0 0 1" rpy="0 0 0"/>
          <axis   xyz="0 0 1"/>
          <mimic joint="driver_joint" multiplier="-2.0" offset="0.5"/>
          <limit  lower="-3.14159" upper="3.14159" effort="10.0" velocity="10.0"/>
        </joint>
      </robot>
    )";

    ASSERT_TRUE(urdf::Model().initString(robot_description_));
    robot_model_ = std::make_unique<RobotModel>(robot_description_);
  }

  std::string robot_description_;
  RobotModel::UniquePtr robot_model_;
};

TEST_F(MimicUrdfTests, RobotModelCachesMimicsAsAffineTransmissions)
{
  // Verify that the URDF mimic relationship gets imported into the analysis as an affine
  // transmission. Per the new API, affine relations are stored per-joint via
  // `affine_transmission_of(joint_id)`, not as a flat vector.
  const auto & analysis = robot_model_->get_default_transmission_analysis();
  const auto & joint_order = analysis.joint_order();

  ASSERT_TRUE(joint_order.contains_key("driver_joint"));
  ASSERT_TRUE(joint_order.contains_key("follower_joint"));

  const auto driver_id = joint_order["driver_joint"];
  const auto follower_id = joint_order["follower_joint"];

  // The two joints should belong to the same affine group.
  EXPECT_EQ(analysis.affine_root_of(driver_id), analysis.affine_root_of(follower_id));

  // The follower's per-joint flat affine relation should be `follower = -2 * driver + 0.5`,
  // which (in flat form) is `follower = -2 * root + 0.5` where root is the driver.
  const auto & follower_flat = analysis.affine_transmission_of(follower_id);
  EXPECT_EQ(follower_flat.target_joint_id, follower_id);
  EXPECT_EQ(follower_flat.source_joint_id, driver_id);  // root is driver_joint
  EXPECT_FLOAT_EQ(follower_flat.multiplier, -2.0F);
  EXPECT_FLOAT_EQ(follower_flat.offset, 0.5F);

  // The driver's flat relation is identity (it's the root).
  const auto & driver_flat = analysis.affine_transmission_of(driver_id);
  EXPECT_EQ(driver_flat.target_joint_id, driver_id);
  EXPECT_EQ(driver_flat.source_joint_id, driver_id);
  EXPECT_FLOAT_EQ(driver_flat.multiplier, 1.0F);
  EXPECT_FLOAT_EQ(driver_flat.offset, 0.0F);
}

// ===========================================================================
// Standalone TransmissionAnalysis URDF import tests (no fixture, embedded URDFs)
// ===========================================================================

TEST(TransmissionAnalysisUrdfImportTests, MimicChainsNormalizeToSingleAffineTransmission)
{
  // A→B→C mimic chain (B mimics A with -2.0/0.5; C mimics B with 3.0/-1.0). After
  // composition, C's flat relation to the root (A) should be -6.0 / 0.5 (per the original
  // test's expectations).
  const std::string robot_description = R"(
      <robot name="mimic_chain_robot">
        <link name="base_link"/>
        <link name="driver_link"/>
        <link name="middle_link"/>
        <link name="follower_link"/>

        <joint name="driver_joint" type="revolute">
          <parent link="base_link"/>
          <child  link="driver_link"/>
          <origin xyz="0 0 0" rpy="0 0 0"/>
          <axis   xyz="0 0 1"/>
          <limit  lower="-3.14159" upper="3.14159" effort="10.0" velocity="10.0"/>
        </joint>

        <joint name="middle_joint" type="revolute">
          <parent link="driver_link"/>
          <child  link="middle_link"/>
          <origin xyz="0 0 1" rpy="0 0 0"/>
          <axis   xyz="0 0 1"/>
          <mimic joint="driver_joint" multiplier="-2.0" offset="0.5"/>
          <limit  lower="-3.14159" upper="3.14159" effort="10.0" velocity="10.0"/>
        </joint>

        <joint name="follower_joint" type="revolute">
          <parent link="middle_link"/>
          <child  link="follower_link"/>
          <origin xyz="0 0 1" rpy="0 0 0"/>
          <axis   xyz="0 0 1"/>
          <mimic joint="middle_joint" multiplier="3.0" offset="-1.0"/>
          <limit  lower="-3.14159" upper="3.14159" effort="10.0" velocity="10.0"/>
        </joint>
      </robot>
    )";

  urdf::Model urdf_model{};
  ASSERT_TRUE(urdf_model.initString(robot_description));

  TransmissionAnalysis analysis{};
  add_mimic_transmissions_to_analysis(analysis, urdf_model);

  const auto & joint_order = analysis.joint_order();
  ASSERT_TRUE(joint_order.contains_key("driver_joint"));
  ASSERT_TRUE(joint_order.contains_key("middle_joint"));
  ASSERT_TRUE(joint_order.contains_key("follower_joint"));

  const auto driver_id = joint_order["driver_joint"];
  const auto middle_id = joint_order["middle_joint"];
  const auto follower_id = joint_order["follower_joint"];

  // All three joints belong to the same affine group, rooted at driver.
  EXPECT_EQ(analysis.affine_root_of(driver_id), driver_id);
  EXPECT_EQ(analysis.affine_root_of(middle_id), driver_id);
  EXPECT_EQ(analysis.affine_root_of(follower_id), driver_id);

  // Composed flat relation for follower: follower = 3 * middle - 1 = 3 * (-2 * driver + 0.5) - 1
  //                                              = -6 * driver + 1.5 - 1 = -6 * driver + 0.5
  const auto & follower_flat = analysis.affine_transmission_of(follower_id);
  EXPECT_EQ(follower_flat.source_joint_id, driver_id);
  EXPECT_EQ(follower_flat.target_joint_id, follower_id);
  EXPECT_NEAR(follower_flat.multiplier, -6.0F, kEpsilon);
  EXPECT_NEAR(follower_flat.offset, 0.5F, kEpsilon);

  // Middle: middle = -2 * driver + 0.5 (unchanged from the direct mimic).
  const auto & middle_flat = analysis.affine_transmission_of(middle_id);
  EXPECT_EQ(middle_flat.source_joint_id, driver_id);
  EXPECT_NEAR(middle_flat.multiplier, -2.0F, kEpsilon);
  EXPECT_NEAR(middle_flat.offset, 0.5F, kEpsilon);
}

TEST(TransmissionAnalysisUrdfImportTests, MimicImportCreatesCanonicalIdForUndefinedSourceJoint)
{
  // A URDF with a mimic referencing a joint that's not defined elsewhere in the URDF. The
  // import should still create a canonical joint id for the undefined source so the affine
  // relationship is representable.
  const std::string robot_description = R"(
      <robot name="mimic_dangling_source_robot">
        <link name="base_link"/>
        <link name="follower_link"/>

        <joint name="follower_joint" type="revolute">
          <parent link="base_link"/>
          <child  link="follower_link"/>
          <origin xyz="0 0 0" rpy="0 0 0"/>
          <axis   xyz="0 0 1"/>
          <mimic joint="undefined_driver_joint" multiplier="2.0" offset="1.0"/>
          <limit  lower="-3.14159" upper="3.14159" effort="10.0" velocity="10.0"/>
        </joint>
      </robot>
    )";

  urdf::Model urdf_model{};
  ASSERT_TRUE(urdf_model.initString(robot_description));

  TransmissionAnalysis analysis{};
  add_mimic_transmissions_to_analysis(analysis, urdf_model);

  const auto & joint_order = analysis.joint_order();
  ASSERT_TRUE(joint_order.contains_key("undefined_driver_joint"));
  ASSERT_TRUE(joint_order.contains_key("follower_joint"));

  const auto driver_id = joint_order["undefined_driver_joint"];
  const auto follower_id = joint_order["follower_joint"];

  EXPECT_EQ(analysis.affine_root_of(driver_id), analysis.affine_root_of(follower_id));

  const auto & follower_flat = analysis.affine_transmission_of(follower_id);
  EXPECT_EQ(follower_flat.source_joint_id, driver_id);
  EXPECT_EQ(follower_flat.target_joint_id, follower_id);
  EXPECT_FLOAT_EQ(follower_flat.multiplier, 2.0F);
  EXPECT_FLOAT_EQ(follower_flat.offset, 1.0F);
}

}  // namespace arm_kinematics
