// Created by Bailey Chessum on 19/04/2026.

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <hardware_interface/hardware_interface/hardware_info.hpp>
#include <hardware_interface/hardware_interface/handle.hpp>
#include <hardware_interface/hardware_interface/loaned_state_interface.hpp>
#include <hardware_interface/hardware_interface/loaned_command_interface.hpp>
#include <hardware_interface/hardware_interface/types/hardware_interface_type_values.hpp>

#include "arm_kinematics/ros2_control/joint_interface_bundle.hpp"

using hardware_interface::InterfaceDescription;
using hardware_interface::InterfaceInfo;
using hardware_interface::LoanedStateInterface;
using hardware_interface::LoanedCommandInterface;
using arm_kinematics::ros2_control::JointInterfaceBundle;
using arm_kinematics::ros2_control::find_joint_interface_bundles;

namespace {

InterfaceDescription make_desc(const std::string & prefix, const std::string & iface_name)
{
  InterfaceInfo info;
  info.name = iface_name;
  info.data_type = "double";
  return InterfaceDescription{prefix, info};
}

LoanedStateInterface make_state(const std::string & prefix, const std::string & iface_name)
{
  auto si = std::make_shared<const hardware_interface::StateInterface>(
    make_desc(prefix, iface_name));
  return LoanedStateInterface{si};
}

LoanedCommandInterface make_command(const std::string & prefix, const std::string & iface_name)
{
  auto ci = std::make_shared<hardware_interface::CommandInterface>(
    make_desc(prefix, iface_name));
  return LoanedCommandInterface{ci, []() {}};
}

}  // namespace

TEST(JointInterfaceBundle, HappyPath_AllRequired)
{
  std::vector<LoanedStateInterface> state_ifaces;
  state_ifaces.push_back(make_state("j1", hardware_interface::HW_IF_POSITION));
  state_ifaces.push_back(make_state("j1", hardware_interface::HW_IF_VELOCITY));
  state_ifaces.push_back(make_state("j2", hardware_interface::HW_IF_POSITION));
  state_ifaces.push_back(make_state("j2", hardware_interface::HW_IF_VELOCITY));

  std::vector<LoanedCommandInterface> cmd_ifaces;
  cmd_ifaces.push_back(make_command("j1", hardware_interface::HW_IF_POSITION));
  cmd_ifaces.push_back(make_command("j2", hardware_interface::HW_IF_POSITION));

  const std::vector<std::string> joints = {"j1", "j2"};

  auto result = find_joint_interface_bundles(
    state_ifaces, cmd_ifaces,
    arm_kinematics::span<const std::string>(joints.data(), joints.size()),
    true, true, hardware_interface::HW_IF_POSITION);

  ASSERT_TRUE(result.has_value()) << result.error().format();
  ASSERT_EQ(result->size(), 2u);

  EXPECT_EQ(result->at(0).joint_name, "j1");
  EXPECT_NE(result->at(0).position, nullptr);
  EXPECT_NE(result->at(0).velocity, nullptr);
  EXPECT_NE(result->at(0).command, nullptr);

  EXPECT_EQ(result->at(1).joint_name, "j2");
  EXPECT_NE(result->at(1).position, nullptr);
  EXPECT_NE(result->at(1).velocity, nullptr);
  EXPECT_NE(result->at(1).command, nullptr);
}

TEST(JointInterfaceBundle, PositionNotRequired_NullptrWhenAbsent)
{
  std::vector<LoanedStateInterface> state_ifaces;
  // Only velocity state — no position
  state_ifaces.push_back(make_state("j1", hardware_interface::HW_IF_VELOCITY));

  std::vector<LoanedCommandInterface> cmd_ifaces;
  cmd_ifaces.push_back(make_command("j1", hardware_interface::HW_IF_POSITION));

  const std::vector<std::string> joints = {"j1"};

  auto result = find_joint_interface_bundles(
    state_ifaces, cmd_ifaces,
    arm_kinematics::span<const std::string>(joints.data(), joints.size()),
    false, false, hardware_interface::HW_IF_POSITION);

  ASSERT_TRUE(result.has_value()) << result.error().format();
  ASSERT_EQ(result->size(), 1u);
  EXPECT_EQ(result->at(0).position, nullptr);   // absent, not required → nullptr
  EXPECT_NE(result->at(0).velocity, nullptr);   // present → non-null
  EXPECT_NE(result->at(0).command, nullptr);
}

TEST(JointInterfaceBundle, PositionRequired_ErrorWhenAbsent)
{
  std::vector<LoanedStateInterface> state_ifaces;
  // No position state for j1

  std::vector<LoanedCommandInterface> cmd_ifaces;
  cmd_ifaces.push_back(make_command("j1", hardware_interface::HW_IF_POSITION));

  const std::vector<std::string> joints = {"j1"};

  auto result = find_joint_interface_bundles(
    state_ifaces, cmd_ifaces,
    arm_kinematics::span<const std::string>(joints.data(), joints.size()),
    true, false, hardware_interface::HW_IF_POSITION);

  ASSERT_FALSE(result.has_value());
  const auto * missing = std::get_if<arm_kinematics::ros2_control::InterfaceLookupError::MissingInterfaces>(
    &result.error().value);
  ASSERT_NE(missing, nullptr);
  ASSERT_EQ(missing->interface_names.size(), 1u);
  EXPECT_EQ(missing->interface_names[0], "j1/position");
}

TEST(JointInterfaceBundle, VelocityRequired_ErrorWhenAbsent)
{
  std::vector<LoanedStateInterface> state_ifaces;
  state_ifaces.push_back(make_state("j1", hardware_interface::HW_IF_POSITION));
  // No velocity state

  std::vector<LoanedCommandInterface> cmd_ifaces;
  cmd_ifaces.push_back(make_command("j1", hardware_interface::HW_IF_POSITION));

  const std::vector<std::string> joints = {"j1"};

  auto result = find_joint_interface_bundles(
    state_ifaces, cmd_ifaces,
    arm_kinematics::span<const std::string>(joints.data(), joints.size()),
    false, true, hardware_interface::HW_IF_POSITION);

  ASSERT_FALSE(result.has_value());
  const auto * missing = std::get_if<arm_kinematics::ros2_control::InterfaceLookupError::MissingInterfaces>(
    &result.error().value);
  ASSERT_NE(missing, nullptr);
  ASSERT_EQ(missing->interface_names.size(), 1u);
  EXPECT_EQ(missing->interface_names[0], "j1/velocity");
}

TEST(JointInterfaceBundle, MissingCommand_AlwaysErrors)
{
  std::vector<LoanedStateInterface> state_ifaces;
  state_ifaces.push_back(make_state("j1", hardware_interface::HW_IF_POSITION));

  std::vector<LoanedCommandInterface> cmd_ifaces;
  // No command interface

  const std::vector<std::string> joints = {"j1"};

  auto result = find_joint_interface_bundles(
    state_ifaces, cmd_ifaces,
    arm_kinematics::span<const std::string>(joints.data(), joints.size()),
    false, false, hardware_interface::HW_IF_POSITION);

  ASSERT_FALSE(result.has_value());
  const auto * missing = std::get_if<arm_kinematics::ros2_control::InterfaceLookupError::MissingInterfaces>(
    &result.error().value);
  ASSERT_NE(missing, nullptr);
  ASSERT_EQ(missing->interface_names.size(), 1u);
  EXPECT_EQ(missing->interface_names[0], "j1/position");
}

TEST(JointInterfaceBundle, AllMissing_CompleteErrorList)
{
  std::vector<LoanedStateInterface> state_ifaces;
  std::vector<LoanedCommandInterface> cmd_ifaces;

  const std::vector<std::string> joints = {"j1", "j2"};

  auto result = find_joint_interface_bundles(
    state_ifaces, cmd_ifaces,
    arm_kinematics::span<const std::string>(joints.data(), joints.size()),
    true, true, hardware_interface::HW_IF_POSITION);

  ASSERT_FALSE(result.has_value());
  const auto * missing = std::get_if<arm_kinematics::ros2_control::InterfaceLookupError::MissingInterfaces>(
    &result.error().value);
  ASSERT_NE(missing, nullptr);
  // position + velocity + command for each of 2 joints = 6 missing
  EXPECT_EQ(missing->interface_names.size(), 6u);
}

TEST(JointInterfaceBundle, VelocityCommandType_MatchedCorrectly)
{
  std::vector<LoanedStateInterface> state_ifaces;
  state_ifaces.push_back(make_state("j1", hardware_interface::HW_IF_POSITION));

  std::vector<LoanedCommandInterface> cmd_ifaces;
  cmd_ifaces.push_back(make_command("j1", hardware_interface::HW_IF_VELOCITY));

  const std::vector<std::string> joints = {"j1"};

  auto result = find_joint_interface_bundles(
    state_ifaces, cmd_ifaces,
    arm_kinematics::span<const std::string>(joints.data(), joints.size()),
    false, false, hardware_interface::HW_IF_VELOCITY);

  ASSERT_TRUE(result.has_value()) << result.error().format();
  EXPECT_NE(result->at(0).command, nullptr);
  EXPECT_EQ(result->at(0).command->get_interface_name(), hardware_interface::HW_IF_VELOCITY);
}
