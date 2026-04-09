//
// Created by Bailey Chessum on 9/4/26.
//
// URDF-driven tests for `RobotModel`'s lazy analysis init, ros2_control transmission plugin
// loading, and the named boundary mapping at the analysis edge.
//

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>

#include <urdf/model.h>

#include "arm_kinematics/common/robot_model.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_analysis_import.hpp"
#include "arm_kinematics/joint_map/ros2_control_transmission_plugin_loader.hpp"

namespace arm_kinematics {

// ===========================================================================
// TransmissionUrdfTests fixture: a URDF with a single ros2_control transmission
// (motor_joint actuator → driven_joint joint, mechanical_reduction=2.0, offset=0.25).
// ===========================================================================

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

        <ros2_control name="TransmissionTestSystem" type="system">
          <hardware>
            <plugin>mock_components/GenericSystem</plugin>
          </hardware>
          <joint name="driven_joint">
            <command_interface name="position"/>
            <command_interface name="velocity"/>
            <state_interface name="position"/>
            <state_interface name="velocity"/>
          </joint>
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
  // Two consecutive calls should return the same instance (lazy init + caching).
  const TransmissionAnalysis & first = robot_model_->get_default_transmission_analysis();
  const TransmissionAnalysis & second = robot_model_->get_default_transmission_analysis();
  EXPECT_EQ(&first, &second);

  // The URDF defined exactly one transmission (main_transmission) and should produce one
  // model + one transmission instance in the analysis.
  ASSERT_EQ(first.models().size(), 1u);
  const auto & transmissions = first.transmissions();
  ASSERT_EQ(transmissions.size(), 1u);

  // The two joints should be registered.
  const auto & joint_order = first.joint_order();
  ASSERT_TRUE(joint_order.contains_key("motor_joint"));
  ASSERT_TRUE(joint_order.contains_key("driven_joint"));

  const auto motor_id = joint_order["motor_joint"];
  const auto driven_id = joint_order["driven_joint"];
  EXPECT_EQ(joint_order.inverse[motor_id], "motor_joint");
  EXPECT_EQ(joint_order.inverse[driven_id], "driven_joint");

  // The transmission's input/output should be the position state interfaces of the
  // respective joints (the import path produces position-interface ids by default).
  const auto & transmission = transmissions.front();
  EXPECT_EQ(transmission.model_id, 0u);
  EXPECT_EQ(transmission.name, "main_transmission");
  EXPECT_EQ(transmission.input_ids.size(), 1u);
  EXPECT_EQ(transmission.output_ids.size(), 1u);
}

TEST_F(TransmissionUrdfTests, RobotModelProvidesSharedRos2ControlTransmissionPluginLoader)
{
  const auto first_loader = robot_model_->get_ros2_control_transmission_plugin_loader();
  const auto second_loader = robot_model_->get_ros2_control_transmission_plugin_loader();

  ASSERT_TRUE(first_loader);
  EXPECT_EQ(first_loader, second_loader);
  EXPECT_TRUE(first_loader->has_plugin_type("transmission_interface/SimpleTransmission"));
  EXPECT_FALSE(first_loader->has_plugin_type("transmission_interface/DoesNotExist"));

  const auto declared_plugin_types = first_loader->get_declared_plugin_types();
  EXPECT_NE(
    std::find(
      declared_plugin_types.begin(),
      declared_plugin_types.end(),
      "transmission_interface/SimpleTransmission"),
    declared_plugin_types.end());
}

TEST_F(TransmissionUrdfTests, Ros2ControlTransmissionPluginLoaderLoadsTransmissionFromTransmissionInfo)
{
  TransmissionAnalysis analysis{};
  const auto loader = robot_model_->get_ros2_control_transmission_plugin_loader();
  const auto transmissions = add_ros2_control_transmissions_to_analysis_dangerous(
    analysis,
    robot_description_,
    loader);

  ASSERT_EQ(transmissions.size(), 1u);

  const auto transmission = loader->load(transmissions.front());
  ASSERT_TRUE(transmission);
  EXPECT_EQ(transmission->num_actuators(), 1u);
  EXPECT_EQ(transmission->num_joints(), 1u);
}

TEST_F(TransmissionUrdfTests, NamedBoundaryMappingStaysAtTransmissionAnalysisEdge)
{
  // Verify that the analysis exposes the named-to-id mapping at its boundary, and that the
  // imported transmission's input/output ids match what you'd get by resolving the names
  // through that boundary mapping.
  const auto & analysis = robot_model_->get_default_transmission_analysis();
  const auto & joint_order = analysis.joint_order();
  const auto & state_interface_order = analysis.state_interface_order();

  ASSERT_TRUE(joint_order.contains_key("motor_joint"));
  ASSERT_TRUE(joint_order.contains_key("driven_joint"));
  ASSERT_FALSE(joint_order.contains_key("unknown_joint"));

  const auto motor_joint_id = joint_order["motor_joint"];
  const auto driven_joint_id = joint_order["driven_joint"];

  // The position interface should be registered for both joints (added by the ros2_control
  // import path).
  const StateInterfaceDefinition motor_position{motor_joint_id, InterfaceId{"position"}};
  const StateInterfaceDefinition driven_position{driven_joint_id, InterfaceId{"position"}};
  ASSERT_TRUE(state_interface_order.contains_key(motor_position));
  ASSERT_TRUE(state_interface_order.contains_key(driven_position));

  const auto motor_position_sid = state_interface_order[motor_position];
  const auto driven_position_sid = state_interface_order[driven_position];

  // The single registered transmission should reference exactly these state interface ids.
  ASSERT_EQ(analysis.transmissions().size(), 1u);
  const auto & transmission = analysis.transmissions().front();
  ASSERT_EQ(transmission.input_ids.size(), 1u);
  ASSERT_EQ(transmission.output_ids.size(), 1u);
  EXPECT_EQ(transmission.input_ids.front(), motor_position_sid);
  EXPECT_EQ(transmission.output_ids.front(), driven_position_sid);
}

}  // namespace arm_kinematics
