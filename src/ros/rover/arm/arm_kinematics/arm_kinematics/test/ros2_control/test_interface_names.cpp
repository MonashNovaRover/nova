// Created by Bailey Chessum on 19/04/2026.

#include <gtest/gtest.h>

#include <hardware_interface/hardware_interface/hardware_info.hpp>
#include <hardware_interface/hardware_interface/types/hardware_interface_type_values.hpp>

#include "arm_kinematics/ros2_control/interface_names.hpp"
#include "arm_kinematics/joint_map/state_interface_definition.hpp"

using arm_kinematics::ros2_control::state_interface_names;
using arm_kinematics::ros2_control::command_interface_names;
using arm_kinematics::NamedStateInterfaceDefinition;
using arm_kinematics::InterfaceId;

TEST(InterfaceNames, StateInterfaceNames_SingleType)
{
  const std::vector<std::string> joints = {"j1", "j2", "j3"};
  const auto names = state_interface_names(
    arm_kinematics::span<const std::string>(joints.data(), joints.size()),
    {hardware_interface::HW_IF_POSITION});

  ASSERT_EQ(names.size(), 3u);
  EXPECT_EQ(names[0], "j1/position");
  EXPECT_EQ(names[1], "j2/position");
  EXPECT_EQ(names[2], "j3/position");
}

TEST(InterfaceNames, StateInterfaceNames_MultipleTypes)
{
  const std::vector<std::string> joints = {"j1", "j2"};
  const auto names = state_interface_names(
    arm_kinematics::span<const std::string>(joints.data(), joints.size()),
    {hardware_interface::HW_IF_POSITION, hardware_interface::HW_IF_VELOCITY});

  ASSERT_EQ(names.size(), 4u);
  EXPECT_EQ(names[0], "j1/position");
  EXPECT_EQ(names[1], "j1/velocity");
  EXPECT_EQ(names[2], "j2/position");
  EXPECT_EQ(names[3], "j2/velocity");
}

TEST(InterfaceNames, StateInterfaceNames_Empty)
{
  const std::vector<std::string> joints;
  const auto names = state_interface_names(
    arm_kinematics::span<const std::string>(joints.data(), joints.size()),
    {hardware_interface::HW_IF_POSITION});
  EXPECT_TRUE(names.empty());
}

TEST(InterfaceNames, StateInterfaceNames_FromDefs)
{
  const std::vector<NamedStateInterfaceDefinition> defs = {
    NamedStateInterfaceDefinition{"shoulder", InterfaceId{"position"}},
    NamedStateInterfaceDefinition{"elbow", InterfaceId{"velocity"}},
  };
  const auto names = state_interface_names(
    arm_kinematics::span<const NamedStateInterfaceDefinition>(defs.data(), defs.size()));

  ASSERT_EQ(names.size(), 2u);
  EXPECT_EQ(names[0], "shoulder/position");
  EXPECT_EQ(names[1], "elbow/velocity");
}

TEST(InterfaceNames, CommandInterfaceNames_NoPrefix)
{
  const std::vector<std::string> joints = {"j1", "j2"};
  const auto names = command_interface_names(
    arm_kinematics::span<const std::string>(joints.data(), joints.size()),
    hardware_interface::HW_IF_POSITION);

  ASSERT_EQ(names.size(), 2u);
  EXPECT_EQ(names[0], "j1/position");
  EXPECT_EQ(names[1], "j2/position");
}

TEST(InterfaceNames, CommandInterfaceNames_WithPrefix)
{
  const std::vector<std::string> joints = {"j1", "j2"};
  const auto names = command_interface_names(
    arm_kinematics::span<const std::string>(joints.data(), joints.size()),
    hardware_interface::HW_IF_POSITION,
    "nova_arm_controller");

  ASSERT_EQ(names.size(), 2u);
  EXPECT_EQ(names[0], "nova_arm_controller/j1/position");
  EXPECT_EQ(names[1], "nova_arm_controller/j2/position");
}

TEST(InterfaceNames, CommandInterfaceNames_EmptyPrefix)
{
  const std::vector<std::string> joints = {"j1"};
  const auto with_empty = command_interface_names(
    arm_kinematics::span<const std::string>(joints.data(), joints.size()),
    hardware_interface::HW_IF_POSITION, "");
  const auto without_prefix = command_interface_names(
    arm_kinematics::span<const std::string>(joints.data(), joints.size()),
    hardware_interface::HW_IF_POSITION);

  EXPECT_EQ(with_empty[0], without_prefix[0]);
  EXPECT_EQ(with_empty[0], "j1/position");
}
