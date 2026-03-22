//
// Created by Bailey Chessum on 17/11/2025.
//

#include <gtest/gtest.h>
#include <urdf_model/model.h>
#include <Eigen/Geometry>

#include "arm_kinematics/forward/utilities/compute_joint_tree.hpp"
#include "arm_kinematics/forward/utilities/compute_frame_tree.hpp"
#include "arm_kinematics/plugins/forward/eigen_forward_kinematics_plugin.hpp"
#include "arm_kinematics/joint_map/affine_joint_map.hpp"
#include "arm_kinematics/joint_map/default_joint_map_builder.hpp"
#include "arm_kinematics/joint_map/joint_map.hpp"
#include "arm_kinematics/joint_map/joint_map_builder.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_analysis_import.hpp"
#include "arm_kinematics/joint_map/transmission_joint_map.hpp"
#include "arm_kinematics/joint_map/transmission_plan.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include <rclcpp/rclcpp.hpp>
#include "arm_kinematics/utilities/reordered.hpp"
#include <algorithm>
#include <chrono>
#include <stdexcept>

using arm_kinematics::ComputeFrameTree;
using arm_kinematics::ComputeJointTree;
using arm_kinematics::FrameDefinitions;
using arm_kinematics::JointMap;
using arm_kinematics::JointMapBuilder;
using arm_kinematics::ForwardKinematicsPlugin;
using arm_kinematics::EigenForwardKinematicsPlugin;
using arm_kinematics::AnalysisTree;
using arm_kinematics::AffineJointMap;
using arm_kinematics::DefaultJointMapBuilder;
using arm_kinematics::KinematicsParams;
using arm_kinematics::Reordered;
using arm_kinematics::RobotModel;
using arm_kinematics::JointQuantity;
using arm_kinematics::TransmissionAnalysis;

namespace {

constexpr double EPSILON = 1e-7;

class TestTransmissionModel final : public arm_kinematics::TransmissionModel
{
public:
  explicit TestTransmissionModel(
    const bool supports_forward = false,
    const bool supports_reverse = false)
  : supports_forward_(supports_forward),
    supports_reverse_(supports_reverse)
  {
  }

  [[nodiscard]] bool can_build(
    arm_kinematics::JointQuantity,
    arm_kinematics::PropagationDirection direction) const noexcept override
  {
    switch (direction) {
      case arm_kinematics::PropagationDirection::Forward:
        return supports_forward_;
      case arm_kinematics::PropagationDirection::Reverse:
        return supports_reverse_;
    }

    return false;
  }

  [[nodiscard]] std::unique_ptr<const arm_kinematics::ComputeTransmission> build(
    arm_kinematics::JointQuantity,
    arm_kinematics::PropagationDirection,
    arm_kinematics::span<const arm_kinematics::JointId>,
    arm_kinematics::span<const arm_kinematics::JointId>) const override
  {
    throw std::logic_error("TestTransmissionModel::build() should not be called in this test");
  }

private:
  bool supports_forward_ = false;
  bool supports_reverse_ = false;
};

class TestComputeTransmission final : public arm_kinematics::ComputeTransmission
{
public:
  TestComputeTransmission(const float multiplier, const float offset)
  : multiplier_(multiplier),
    offset_(offset)
  {
  }

  void compute(
    arm_kinematics::span<const float> inputs,
    arm_kinematics::span<float> outputs,
    arm_kinematics::span<float>) const override
  {
    outputs[0] = inputs[0] * multiplier_ + offset_;
  }

  [[nodiscard]] std::unique_ptr<arm_kinematics::ComputeTransmission> clone() const override
  {
    return std::make_unique<TestComputeTransmission>(multiplier_, offset_);
  }

private:
  float multiplier_ = 1.0F;
  float offset_ = 0.0F;
};

class TestRuntimeTransmissionModel final : public arm_kinematics::TransmissionModel
{
public:
  explicit TestRuntimeTransmissionModel(const float multiplier, const float offset)
  : multiplier_(multiplier),
    offset_(offset)
  {
  }

  [[nodiscard]] bool can_build(
    arm_kinematics::JointQuantity,
    arm_kinematics::PropagationDirection direction) const noexcept override
  {
    return direction == arm_kinematics::PropagationDirection::Forward;
  }

  [[nodiscard]] std::unique_ptr<const arm_kinematics::ComputeTransmission> build(
    arm_kinematics::JointQuantity,
    arm_kinematics::PropagationDirection,
    arm_kinematics::span<const arm_kinematics::JointId>,
    arm_kinematics::span<const arm_kinematics::JointId>) const override
  {
    return std::make_unique<TestComputeTransmission>(multiplier_, offset_);
  }

private:
  float multiplier_ = 1.0F;
  float offset_ = 0.0F;
};

static void ExpectVectorNear(const Eigen::Vector3f & actual,
                             const Eigen::Vector3f & expected,
                             const char * message = "", double tol = EPSILON)
{
  EXPECT_NEAR(actual.x(), expected.x(), tol) << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix();
  EXPECT_NEAR(actual.y(), expected.y(), tol) << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix();
  EXPECT_NEAR(actual.z(), expected.z(), tol) << message << "\n" << actual.matrix() << "\n vs \n" << expected.matrix();
}

TEST(JointMapStage1Tests, DefaultConstructedJointMapIsInvalid)
{
  JointMap joint_map{};

  EXPECT_FALSE(joint_map.valid());
  EXPECT_EQ(joint_map.input_count(), 0);
  EXPECT_EQ(joint_map.output_count(), 0);

  std::vector<double> inputs{};
  std::vector<float> outputs{};
  EXPECT_THROW(joint_map.map(inputs, outputs), std::logic_error);
}

TEST(JointMapStage1Tests, AffineJointMapPreservesPureReorderBehavior)
{
  AffineJointMap map(
    {"joint_a", "joint_b", "joint_c"},
    {"joint_c", "joint_a", "joint_b"});

  std::vector<double> inputs{1.0, 2.0, 3.0};
  std::vector<float> outputs(3);
  map.map(inputs, outputs);

  ASSERT_EQ(map.input_count(), 3);
  ASSERT_EQ(map.output_count(), 3);
  EXPECT_NEAR(outputs[0], 3.0, EPSILON);
  EXPECT_NEAR(outputs[1], 1.0, EPSILON);
  EXPECT_NEAR(outputs[2], 2.0, EPSILON);
}

TEST(JointMapStage1Tests, AffineJointMapPreservesMimicBehavior)
{
  std::map<std::string, std::shared_ptr<urdf::JointMimic>> mimic_joints{};
  auto mimic = std::make_shared<urdf::JointMimic>();
  mimic->joint_name = "driver";
  mimic->multiplier = -1.0;
  mimic->offset = 0.25;
  mimic_joints["follower"] = mimic;

  AffineJointMap map(
    {"driver"},
    {"driver", "follower"},
    mimic_joints);

  std::vector<double> inputs{2.0};
  std::vector<float> outputs(2);
  map.map(inputs, outputs);

  ASSERT_EQ(map.input_count(), 1);
  ASSERT_EQ(map.output_count(), 2);
  EXPECT_NEAR(outputs[0], 2.0, EPSILON);
  EXPECT_NEAR(outputs[1], -1.75, EPSILON);
}

TEST(JointMapStage1Tests, JointMapCopyRetainsBehavior)
{
  JointMap original(AffineJointMap(
    {"driver"},
    {"driver", "driver"}));

  JointMap copy = original;

  ASSERT_TRUE(original.valid());
  ASSERT_TRUE(copy.valid());
  ASSERT_EQ(copy.input_count(), 1);
  ASSERT_EQ(copy.output_count(), 2);

  std::vector<double> inputs{4.0};
  std::vector<float> original_outputs(2);
  std::vector<float> copy_outputs(2);

  original.map(inputs, original_outputs);
  copy.map(inputs, copy_outputs);

  EXPECT_EQ(original_outputs, copy_outputs);
}

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

        <ros2_control>
        </ros2_control>
      </robot>
    )";

    ASSERT_TRUE(urdf::Model().initString(robot_description_));
    robot_model_ = std::make_unique<RobotModel>(robot_description_);
  }

  std::string robot_description_;
  RobotModel::UniquePtr robot_model_;
};

TEST_F(MimicUrdfTests, DefaultJointMapBuilderMatchesRobotModelBuilder)
{
  DefaultJointMapBuilder builder{};
  builder.with_urdf(robot_model_->get_urdf_model());
  builder.with_transmissions(robot_description_, rclcpp::get_logger("joint_map_test"));

  const auto default_map = builder.build(
    {"driver_joint"},
    {"driver_joint", "follower_joint"});
  const auto robot_model_map = robot_model_->get_joint_map_builder().build(
    {"driver_joint"},
    {"driver_joint", "follower_joint"});

  std::vector<double> inputs{1.5};
  std::vector<float> default_outputs(2);
  std::vector<float> robot_model_outputs(2);

  default_map.map(inputs, default_outputs);
  robot_model_map.map(inputs, robot_model_outputs);

  EXPECT_EQ(default_outputs, robot_model_outputs);
  EXPECT_NEAR(default_outputs[0], 1.5, EPSILON);
  EXPECT_NEAR(default_outputs[1], -2.5, EPSILON);
}

TEST_F(MimicUrdfTests, RobotModelCachesMimicsAsAffineTransmissions)
{
  const auto & analysis = robot_model_->get_transmission_analysis();
  const auto & joint_ids = analysis.joint_order();
  const auto & affine_transmissions = analysis.affine_transmissions();

  ASSERT_EQ(affine_transmissions.size(), 1u);
  ASSERT_TRUE(joint_ids.contains_key("driver_joint"));
  ASSERT_TRUE(joint_ids.contains_key("follower_joint"));

  const auto & affine_transmission = affine_transmissions.front();
  EXPECT_EQ(affine_transmission.source_joint_id, joint_ids["driver_joint"]);
  EXPECT_EQ(affine_transmission.target_joint_id, joint_ids["follower_joint"]);
  EXPECT_FLOAT_EQ(affine_transmission.multiplier, -2.0F);
  EXPECT_FLOAT_EQ(affine_transmission.offset, 0.5F);
}

TEST(TransmissionAnalysisTests, MimicChainsNormalizeToSingleAffineTransmission)
{
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

  const auto & joint_ids = analysis.joint_order();
  const auto & affine_transmissions = analysis.affine_transmissions();

  ASSERT_EQ(affine_transmissions.size(), 2u);

  const auto follower_it = std::find_if(
    affine_transmissions.begin(),
    affine_transmissions.end(),
    [&joint_ids](const TransmissionAnalysis::AffineTransmission & affine_transmission) {
      return affine_transmission.target_joint_id == joint_ids["follower_joint"];
    });

  ASSERT_NE(follower_it, affine_transmissions.end());
  EXPECT_EQ(follower_it->source_joint_id, joint_ids["driver_joint"]);
  EXPECT_FLOAT_EQ(follower_it->multiplier, -6.0F);
  EXPECT_FLOAT_EQ(follower_it->offset, 0.5F);
}

TEST(TransmissionAnalysisTests, MimicImportCreatesCanonicalIdForUndefinedSourceJoint)
{
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

  const auto & joint_ids = analysis.joint_order();
  const auto & affine_transmissions = analysis.affine_transmissions();

  ASSERT_TRUE(joint_ids.contains_key("undefined_driver_joint"));
  ASSERT_TRUE(joint_ids.contains_key("follower_joint"));
  ASSERT_EQ(affine_transmissions.size(), 1u);

  const auto & affine_transmission = affine_transmissions.front();
  EXPECT_EQ(affine_transmission.source_joint_id, joint_ids["undefined_driver_joint"]);
  EXPECT_EQ(affine_transmission.target_joint_id, joint_ids["follower_joint"]);
  EXPECT_FLOAT_EQ(affine_transmission.multiplier, 2.0F);
  EXPECT_FLOAT_EQ(affine_transmission.offset, 1.0F);
}

class TransmissionUrdfTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    robot_description_ = R"(
      <robot name="transmission_robot">
        <link name="base_link"/>
        <link name="driven_link"/>

        <joint name="driven_joint" type="revolute">
          <parent link="base_link"/>
          <child  link="driven_link"/>
          <origin xyz="0 0 0" rpy="0 0 0"/>
          <axis   xyz="0 0 1"/>
          <limit  lower="-3.14159" upper="3.14159" effort="10.0" velocity="10.0"/>
        </joint>

        <ros2_control>
          <transmission name="main_transmission">
            <plugin>transmission_interface/SimpleTransmission</plugin>
            <joint name="driven_joint" role="joint">
              <mechanical_reduction>2.0</mechanical_reduction>
              <offset>0.25</offset>
            </joint>
            <actuator name="motor_joint" role="actuator">
              <mechanical_reduction>1.0</mechanical_reduction>
              <offset>0.0</offset>
            </actuator>
          </transmission>
        </ros2_control>
      </robot>
    )";

    ASSERT_TRUE(urdf::Model().initString(robot_description_));
    robot_model_ = std::make_unique<RobotModel>(robot_description_);
  }

  std::string robot_description_;
  RobotModel::UniquePtr robot_model_;
};

TEST_F(TransmissionUrdfTests, RobotModelCachesTransmissionAnalysis)
{
  const TransmissionAnalysis & first = robot_model_->get_transmission_analysis();
  const TransmissionAnalysis & second = robot_model_->get_transmission_analysis();
  const auto & joint_ids = first.joint_order();
  const auto & transmissions = first.transmissions();

  EXPECT_EQ(&first, &second);
  ASSERT_EQ(first.models().size(), 1u);
  ASSERT_EQ(transmissions.size(), 1u);
  ASSERT_TRUE(joint_ids.contains_key("motor_joint"));
  ASSERT_TRUE(joint_ids.contains_key("driven_joint"));

  const auto motor_id = joint_ids["motor_joint"];
  const auto driven_id = joint_ids["driven_joint"];
  EXPECT_EQ(joint_ids.inverse[motor_id], "motor_joint");
  EXPECT_EQ(joint_ids.inverse[driven_id], "driven_joint");

  const auto & transmission = transmissions.front();
  ASSERT_EQ(transmission.model_id, 0u);
  ASSERT_EQ(transmission.input_joint_ids.size(), 1u);
  ASSERT_EQ(transmission.output_joint_ids.size(), 1u);
  EXPECT_EQ(transmission.input_joint_ids.front(), motor_id);
  EXPECT_EQ(transmission.output_joint_ids.front(), driven_id);
  EXPECT_EQ(transmission.name, "main_transmission");
}

TEST_F(TransmissionUrdfTests, BuildExpectedDoesNotClaimGroupedTransmissionSupportYet)
{
  const auto result = robot_model_->get_joint_map_builder().build_expected(
    {"motor_joint"},
    {"driven_joint"},
    JointQuantity::Position);

  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("No affine plan found"), std::string::npos);
}

TEST_F(TransmissionUrdfTests, NamedBoundaryMappingStaysAtTransmissionAnalysisEdge)
{
  const auto & analysis = robot_model_->get_transmission_analysis();
  const auto & joint_ids = analysis.joint_order();

  ASSERT_TRUE(joint_ids.contains_key("motor_joint"));
  ASSERT_TRUE(joint_ids.contains_key("driven_joint"));
  ASSERT_FALSE(joint_ids.contains_key("unknown_joint"));

  const std::vector<std::string> request_inputs{"motor_joint"};
  const std::vector<std::string> request_outputs{"driven_joint"};
  const auto input_ids = joint_ids * request_inputs;
  const auto output_ids = joint_ids * request_outputs;

  ASSERT_EQ(input_ids.size(), 1u);
  ASSERT_EQ(output_ids.size(), 1u);

  const auto & transmission = analysis.transmissions().front();
  EXPECT_EQ(transmission.input_joint_ids, input_ids);
  EXPECT_EQ(transmission.output_joint_ids, output_ids);
}

TEST(TransmissionAnalysisTests, NamedAddTransmissionImmediatelyConvertsToJointIds)
{
  TransmissionAnalysis analysis{};
  const auto model_id = analysis.add_model(std::make_unique<TestTransmissionModel>());

  const std::vector<std::string> inputs{"motor_joint"};
  const std::vector<std::string> outputs{"driven_joint"};
  analysis.add_transmission(
    model_id,
    arm_kinematics::span<const std::string>(inputs),
    arm_kinematics::span<const std::string>(outputs),
    "named_seed");

  const auto & joint_ids = analysis.joint_order();
  ASSERT_TRUE(joint_ids.contains_key("motor_joint"));
  ASSERT_TRUE(joint_ids.contains_key("driven_joint"));
  ASSERT_EQ(analysis.transmissions().size(), 1u);

  const auto & transmission = analysis.transmissions().front();
  EXPECT_EQ(transmission.model_id, model_id);
  EXPECT_EQ(transmission.input_joint_ids.front(), joint_ids["motor_joint"]);
  EXPECT_EQ(transmission.output_joint_ids.front(), joint_ids["driven_joint"]);
  EXPECT_EQ(transmission.name, "named_seed");
}

TEST(TransmissionAnalysisTests, IndexedAddTransmissionRejectsUnknownJointIds)
{
  TransmissionAnalysis analysis{};
  const auto model_id = analysis.add_model(std::make_unique<TestTransmissionModel>());

  EXPECT_THROW(
    analysis.add_transmission(model_id, std::vector<arm_kinematics::JointId>{0}, std::vector<arm_kinematics::JointId>{1}),
    std::invalid_argument);
}

TEST(JointMapStage2Tests, DefaultJointMapBuilderBuildsDirectTransmissionPlan)
{
  DefaultJointMapBuilder builder{};
  builder.with_transmission_model(std::make_unique<TestTransmissionModel>(true, false));

  auto & analysis = const_cast<TransmissionAnalysis &>(builder.get_transmission_analysis());
  const std::vector<std::string> input_names{"motor_joint"};
  const std::vector<std::string> output_names{"driven_joint"};
  analysis.add_transmission(
    0,
    arm_kinematics::span<const std::string>(input_names),
    arm_kinematics::span<const std::string>(output_names),
    "direct");

  const auto & joint_order = analysis.joint_order();
  const std::vector<arm_kinematics::JointId> input_ids{joint_order["motor_joint"]};
  const std::vector<arm_kinematics::JointId> output_ids{joint_order["driven_joint"]};
  const auto plan = arm_kinematics::make_transmission_plan_expected(
    analysis,
    arm_kinematics::span<const arm_kinematics::JointId>(input_ids),
    arm_kinematics::span<const arm_kinematics::JointId>(output_ids),
    JointQuantity::Position);

  ASSERT_TRUE(plan.has_value());
  ASSERT_EQ(plan.value().input_joint_ids.size(), 1u);
  ASSERT_EQ(plan.value().output_joint_ids.size(), 1u);
  ASSERT_EQ(plan.value().stages.size(), 1u);
  EXPECT_EQ(plan.value().stages.front().group_id, 0u);
  EXPECT_EQ(plan.value().stages.front().direction, arm_kinematics::PropagationDirection::Forward);
}

TEST(JointMapStage2Tests, CompilesAndExecutesDirectGroupedTransmissionPlan)
{
  TransmissionAnalysis analysis{};
  const auto model_id = analysis.add_model(std::make_unique<TestRuntimeTransmissionModel>(2.0F, 0.25F));

  const std::vector<std::string> input_names{"motor_joint"};
  const std::vector<std::string> output_names{"driven_joint"};
  analysis.add_transmission(
    model_id,
    arm_kinematics::span<const std::string>(input_names),
    arm_kinematics::span<const std::string>(output_names),
    "direct");

  const auto & joint_order = analysis.joint_order();
  const std::vector<arm_kinematics::JointId> input_ids{joint_order["motor_joint"]};
  const std::vector<arm_kinematics::JointId> output_ids{joint_order["driven_joint"]};
  const auto plan = arm_kinematics::make_transmission_plan_expected(
    analysis,
    arm_kinematics::span<const arm_kinematics::JointId>(input_ids),
    arm_kinematics::span<const arm_kinematics::JointId>(output_ids),
    JointQuantity::Position);

  ASSERT_TRUE(plan.has_value());

  const auto compiled_plan = arm_kinematics::compile_transmission_plan_expected(
    analysis,
    *plan,
    JointQuantity::Position);

  ASSERT_TRUE(compiled_plan.has_value());
  ASSERT_EQ(compiled_plan->input_count, 1u);
  ASSERT_EQ(compiled_plan->output_count, 1u);
  ASSERT_EQ(compiled_plan->stages.size(), 1u);

  arm_kinematics::TransmissionJointMap joint_map(std::move(compiled_plan.value()));
  std::vector<double> inputs{1.5};
  std::vector<float> outputs(1, 0.0F);
  joint_map.map(inputs, outputs);

  EXPECT_NEAR(outputs[0], 3.25F, EPSILON);
}

TEST(TransmissionAnalysisTests, AffinePlannerBuildsIdentityPlanForDirectInputJoint)
{
  TransmissionAnalysis analysis{};
  const auto input_joint_id = analysis.ensure_joint_id("input_joint");

  const std::vector<arm_kinematics::JointId> input_ids{input_joint_id};
  const std::vector<arm_kinematics::JointId> output_ids{input_joint_id};
  const auto plan = arm_kinematics::make_affine_plan_expected(
    analysis,
    arm_kinematics::span<const arm_kinematics::JointId>(input_ids),
    arm_kinematics::span<const arm_kinematics::JointId>(output_ids));

  ASSERT_TRUE(plan.has_value());
  ASSERT_EQ(plan->stages.size(), 1u);
  EXPECT_EQ(plan->stages.front().source_joint_id, input_joint_id);
  EXPECT_EQ(plan->stages.front().target_joint_id, input_joint_id);
  EXPECT_EQ(plan->stages.front().source_input_index, 0u);
  EXPECT_FLOAT_EQ(plan->stages.front().multiplier, 1.0F);
  EXPECT_FLOAT_EQ(plan->stages.front().offset, 0.0F);
}

TEST(TransmissionAnalysisTests, AffinePlannerBuildsPlanForMimicDerivedAffineTransmission)
{
  TransmissionAnalysis analysis{};
  analysis.add_affine_transmission("driver_joint", "follower_joint", -2.0F, 0.5F);

  const auto & joint_ids = analysis.joint_order();
  const std::vector<arm_kinematics::JointId> input_ids{joint_ids["driver_joint"]};
  const std::vector<arm_kinematics::JointId> output_ids{joint_ids["follower_joint"]};
  const auto plan = arm_kinematics::make_affine_plan_expected(
    analysis,
    arm_kinematics::span<const arm_kinematics::JointId>(input_ids),
    arm_kinematics::span<const arm_kinematics::JointId>(output_ids));

  ASSERT_TRUE(plan.has_value());
  ASSERT_EQ(plan->stages.size(), 1u);
  EXPECT_EQ(plan->stages.front().source_joint_id, joint_ids["driver_joint"]);
  EXPECT_EQ(plan->stages.front().target_joint_id, joint_ids["follower_joint"]);
  EXPECT_EQ(plan->stages.front().source_input_index, 0u);
  EXPECT_FLOAT_EQ(plan->stages.front().multiplier, -2.0F);
  EXPECT_FLOAT_EQ(plan->stages.front().offset, 0.5F);
}

TEST(TransmissionAnalysisTests, AffinePlannerComposesAffineTransmissionChains)
{
  TransmissionAnalysis analysis{};
  analysis.add_affine_transmission("driver_joint", "middle_joint", -2.0F, 0.5F);
  analysis.add_affine_transmission("middle_joint", "follower_joint", 3.0F, -1.0F);

  const auto & joint_ids = analysis.joint_order();
  const std::vector<arm_kinematics::JointId> input_ids{joint_ids["driver_joint"]};
  const std::vector<arm_kinematics::JointId> output_ids{joint_ids["follower_joint"]};
  const auto plan = arm_kinematics::make_affine_plan_expected(
    analysis,
    arm_kinematics::span<const arm_kinematics::JointId>(input_ids),
    arm_kinematics::span<const arm_kinematics::JointId>(output_ids));

  ASSERT_TRUE(plan.has_value());
  ASSERT_EQ(plan->stages.size(), 1u);
  EXPECT_EQ(plan->stages.front().source_joint_id, joint_ids["driver_joint"]);
  EXPECT_EQ(plan->stages.front().target_joint_id, joint_ids["follower_joint"]);
  EXPECT_EQ(plan->stages.front().source_input_index, 0u);
  EXPECT_FLOAT_EQ(plan->stages.front().multiplier, -6.0F);
  EXPECT_FLOAT_EQ(plan->stages.front().offset, 0.5F);
}

TEST(TransmissionAnalysisTests, AffineJointMapExecutesCompiledAffinePlan)
{
  TransmissionAnalysis analysis{};
  analysis.add_affine_transmission("driver_joint", "follower_joint", -2.0F, 0.5F);

  const auto & joint_ids = analysis.joint_order();
  const std::vector<arm_kinematics::JointId> input_ids{joint_ids["driver_joint"]};
  const std::vector<arm_kinematics::JointId> output_ids{joint_ids["driver_joint"], joint_ids["follower_joint"]};
  const auto plan = arm_kinematics::make_affine_plan_expected(
    analysis,
    arm_kinematics::span<const arm_kinematics::JointId>(input_ids),
    arm_kinematics::span<const arm_kinematics::JointId>(output_ids));

  ASSERT_TRUE(plan.has_value());

  AffineJointMap map(*plan);
  std::vector<double> inputs{1.5};
  std::vector<float> outputs(2);
  map.map(inputs, outputs);

  ASSERT_EQ(map.input_count(), 1u);
  ASSERT_EQ(map.output_count(), 2u);
  EXPECT_NEAR(outputs[0], 1.5, EPSILON);
  EXPECT_NEAR(outputs[1], -2.5, EPSILON);
}

TEST(TransmissionAnalysisTests, AffineJointMapExecutesCompiledAffineChainPlan)
{
  TransmissionAnalysis analysis{};
  analysis.add_affine_transmission("driver_joint", "middle_joint", -2.0F, 0.5F);
  analysis.add_affine_transmission("middle_joint", "follower_joint", 3.0F, -1.0F);

  const auto & joint_ids = analysis.joint_order();
  const std::vector<arm_kinematics::JointId> input_ids{joint_ids["driver_joint"]};
  const std::vector<arm_kinematics::JointId> output_ids{joint_ids["follower_joint"]};
  const auto plan = arm_kinematics::make_affine_plan_expected(
    analysis,
    arm_kinematics::span<const arm_kinematics::JointId>(input_ids),
    arm_kinematics::span<const arm_kinematics::JointId>(output_ids));

  ASSERT_TRUE(plan.has_value());

  AffineJointMap map(*plan);
  std::vector<double> inputs{1.5};
  std::vector<float> outputs(1);
  map.map(inputs, outputs);

  ASSERT_EQ(map.input_count(), 1u);
  ASSERT_EQ(map.output_count(), 1u);
  EXPECT_NEAR(outputs[0], -8.5, EPSILON);
}

TEST(TransmissionAnalysisTests, AffineJointMapConstructsFromTransmissionAnalysisViaAffinePlanner)
{
  TransmissionAnalysis analysis{};
  analysis.add_affine_transmission("driver_joint", "follower_joint", -2.0F, 0.5F);

  AffineJointMap map(
    {"driver_joint"},
    {"driver_joint", "follower_joint"},
    analysis);

  std::vector<double> inputs{1.5};
  std::vector<float> outputs(2);
  map.map(inputs, outputs);

  ASSERT_EQ(map.input_count(), 1u);
  ASSERT_EQ(map.output_count(), 2u);
  EXPECT_NEAR(outputs[0], 1.5, EPSILON);
  EXPECT_NEAR(outputs[1], -2.5, EPSILON);
}

TEST(TransmissionAnalysisTests, AffinePlannerRejectsUnresolvedOutputJoint)
{
  TransmissionAnalysis analysis{};
  const auto input_joint_id = analysis.ensure_joint_id("input_joint");
  const auto output_joint_id = analysis.ensure_joint_id("unresolved_output_joint");

  const std::vector<arm_kinematics::JointId> input_ids{input_joint_id};
  const std::vector<arm_kinematics::JointId> output_ids{output_joint_id};
  const auto plan = arm_kinematics::make_affine_plan_expected(
    analysis,
    arm_kinematics::span<const arm_kinematics::JointId>(input_ids),
    arm_kinematics::span<const arm_kinematics::JointId>(output_ids));

  ASSERT_FALSE(plan.has_value());
  EXPECT_NE(plan.error().find("No affine plan found"), std::string::npos);
}

TEST(TransmissionAnalysisTests, AffinePlannerRejectsAmbiguousAffineTargets)
{
  TransmissionAnalysis analysis{};
  analysis.add_affine_transmission("driver_a_joint", "follower_joint", 2.0F, 0.0F);
  analysis.add_affine_transmission("driver_b_joint", "follower_joint", 3.0F, 0.0F);

  const auto & joint_ids = analysis.joint_order();
  const std::vector<arm_kinematics::JointId> input_ids{
    joint_ids["driver_a_joint"],
    joint_ids["driver_b_joint"]
  };
  const std::vector<arm_kinematics::JointId> output_ids{joint_ids["follower_joint"]};
  const auto plan = arm_kinematics::make_affine_plan_expected(
    analysis,
    arm_kinematics::span<const arm_kinematics::JointId>(input_ids),
    arm_kinematics::span<const arm_kinematics::JointId>(output_ids));

  ASSERT_FALSE(plan.has_value());
  EXPECT_NE(plan.error().find("Ambiguous affine plan"), std::string::npos);
}

TEST(JointMapStage2Tests, BuildExpectedDoesNotClaimGroupedTransmissionSupportYet)
{
  DefaultJointMapBuilder builder{};
  builder.with_transmission_model(std::make_unique<TestTransmissionModel>(true, false));

  auto & analysis = const_cast<TransmissionAnalysis &>(builder.get_transmission_analysis());
  const std::vector<std::string> input_names{"motor_joint"};
  const std::vector<std::string> output_names{"driven_joint"};
  analysis.add_transmission(
    0,
    arm_kinematics::span<const std::string>(input_names),
    arm_kinematics::span<const std::string>(output_names),
    "direct");

  const auto result = builder.build_expected(input_names, output_names, JointQuantity::Position);

  ASSERT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("No affine plan found"), std::string::npos);
}

// Small helper for comparing isometries
static void ExpectIsometryNear(const Eigen::Isometry3f & actual,
                               const Eigen::Isometry3f & expected,
                               const char * message = "", double tol = EPSILON)
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
    ASSERT_TRUE(urdf::Model().initString(robot_description_));
    robot_model_ = std::make_unique<RobotModel>(robot_description_);

    node_ = std::make_shared<rclcpp::Node>("test_eigen_fk_mapper");
    logger_ = node_->get_logger();
    kinematics_params_ = std::make_shared<KinematicsParams>(*node_);
  }

  void TearDown() override {
    node_.reset();
  }

  std::string robot_description_;
  RobotModel::UniquePtr robot_model_;
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
  auto init_result = plugin->initialize(*node_, *robot_model_, kinematics_params_);

  ASSERT_TRUE(init_result) << "Failed to initialize plugin";

  const float theta = M_PI / 2.0;
  const float d     = 0.5;
  std::vector<float> joint_states{ d, theta };

  RCLCPP_INFO(node_->get_logger(), "Creating FK Tree");
  auto anal = AnalysisTree(plugin->get_robot_model().get_urdf_model());

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
  auto init_result = plugin->initialize(*node_, *robot_model_, kinematics_params_);

  ASSERT_TRUE(init_result) << "Failed to initialize plugin";

  const double theta = M_PI / 2.0;
  const double d     = 0.5;
  std::vector<double> joint_states{ d, theta };

  auto weird_joint_map = plugin->get_joint_map_builder().build({"joint1", "joint2"}, {"joint1", "joint2", "joint1", "joint 1", "joint 2"});
  std::vector<float> mapped_joint_states_weird(weird_joint_map.output_count());
  weird_joint_map.map(joint_states, mapped_joint_states_weird);

  RCLCPP_INFO(node_->get_logger(), "Creating FK Tree");
  auto [tree_, order] = plugin->make_tree(joint_names, std::string("base_link"), frames);

  ASSERT_TRUE(order.size() > 0) << "Failed to create tree with order";

  auto * tree = dynamic_cast<EigenForwardKinematicsPlugin::TreeImpl *>(tree_.get());
  ASSERT_TRUE(tree) << "Failed to create tree";

  std::vector<float> mapped_joint_states(tree->get_joint_map().output_count());
  tree->get_joint_map().map(joint_states, mapped_joint_states);
  EXPECT_NEAR(mapped_joint_states[0], joint_states[0], EPSILON) << "joint 0 value mapped incorrectly";
  EXPECT_NEAR(mapped_joint_states[1], joint_states[1], EPSILON) << "joint 1 value mapped incorrectly";

  EXPECT_EQ(tree->get_tree().get_tree().poses.size(), 2) << "joint tree poses are the wrong size";
  EXPECT_EQ(tree->get_joint_map().input_count(), 2) << "joint map is the wrong size";
  EXPECT_EQ(tree->get_joint_map().output_count(), 2) << "joint map is the wrong size";
  EXPECT_TRUE(tree->get_joint_map().valid()) << "joint map should be valid";
  EXPECT_EQ(tree->get_mapped_joint_states().size(), 2) << "mapped joint states are the wrong size";

  EXPECT_EQ(tree->get_tree().get_parents()[0], 0) << "frame 0 has the wrong parent";
  EXPECT_EQ(tree->get_tree().get_parents()[1], 1) << "frame 1 has the wrong parent";

  RCLCPP_INFO(node_->get_logger(), "Allocating pose outputs");
  arm_kinematics::Isometry3fVector link_poses(output_count, Eigen::Isometry3f::Identity());
  RCLCPP_INFO(node_->get_logger(), "Performing FK");

  tree->position_fk(joint_states, link_poses);

  EXPECT_NEAR(tree->get_mapped_joint_states()[0], joint_states[0], EPSILON) << "joint 0 value mapped incorrectly";
  EXPECT_NEAR(tree->get_mapped_joint_states()[1], joint_states[1], EPSILON) << "joint 1 value mapped incorrectly";


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
  auto init_result = plugin->initialize(*node_, *robot_model_, kinematics_params_);

  ASSERT_TRUE(init_result) << "Failed to initialize plugin";

  const double theta = M_PI / 2.0;
  const double d     = 0.5;
  std::vector<double> joint_states{ d, theta };

  RCLCPP_INFO(node_->get_logger(), "Creating FK Tree");
  auto anal = AnalysisTree(plugin->get_robot_model().get_urdf_model());

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
  std::vector<float> joint_states_mapped(joint_map.output_count());
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
  j2_truth.linear() = Eigen::AngleAxisf(M_PI / 2.0, Eigen::Vector3f(0, 0, 1)).toRotationMatrix();
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
    ASSERT_TRUE(urdf::Model().initString(robot_description_));
    robot_model_ = std::make_unique<RobotModel>(robot_description_);

    node_ = std::make_shared<rclcpp::Node>("test_eigen_fk_mapper");
    logger_ = node_->get_logger();
    kinematics_params_ = std::make_shared<KinematicsParams>(*node_);

    plugin_ = std::make_shared<EigenForwardKinematicsPlugin>();
    init_result_ = plugin_->initialize(*node_, *robot_model_, kinematics_params_);
  }

  void TearDown() override {
    node_.reset();
  }

  std::string robot_description_;
  RobotModel::UniquePtr robot_model_;
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

  constexpr size_t calculations_per_iteration = 131072;

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
    for (size_t k = 0; k < calculations_per_iteration; ++k) {
      tree->position_fk({}, actual);
    }
    auto end   = Clock::now();
    auto dur   = end - start;

    const auto us = std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
    std::cout << "Took " << us << " µs\n\n";
    us_total += us;
  }

  const size_t total_count = calculations_per_iteration * frame_names_.size();

  std::cout << "Average of " << static_cast<long double>(us_total) / static_cast<long double>(total_count) << " µs per FK calculation\n";
  std::cout << "Average of " << 1e6 * static_cast<long double>(total_count) / static_cast<long double>(us_total) << " hz\n\n";
}
