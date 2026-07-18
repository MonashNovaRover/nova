// Created by Bailey Chessum on 19/04/2026.

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <hardware_interface/hardware_interface/hardware_info.hpp>
#include <hardware_interface/hardware_interface/handle.hpp>
#include <hardware_interface/hardware_interface/loaned_state_interface.hpp>
#include <hardware_interface/hardware_interface/loaned_command_interface.hpp>
#include <hardware_interface/hardware_interface/types/hardware_interface_type_values.hpp>

#include "arm_kinematics/ros2_control/interface_refs.hpp"
#include "arm_kinematics/joint_map/state_interface_definition.hpp"

using hardware_interface::InterfaceDescription;
using hardware_interface::InterfaceInfo;
using hardware_interface::LoanedStateInterface;
using hardware_interface::LoanedCommandInterface;
using arm_kinematics::NamedStateInterfaceDefinition;
using arm_kinematics::InterfaceId;
using arm_kinematics::ros2_control::find_state_interface_refs;
using arm_kinematics::ros2_control::find_command_interface_refs;

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

// ---------------------------------------------------------------------------
// find_state_interface_refs
// ---------------------------------------------------------------------------

TEST(InterfaceRefs, StateRefs_HappyPath_OrderPreserved)
{
  std::vector<LoanedStateInterface> ifaces;
  ifaces.push_back(make_state("elbow", "position"));    // index 0
  ifaces.push_back(make_state("shoulder", "position")); // index 1

  const std::vector<NamedStateInterfaceDefinition> defs = {
    NamedStateInterfaceDefinition{"shoulder", InterfaceId{"position"}},
    NamedStateInterfaceDefinition{"elbow", InterfaceId{"position"}},
  };

  auto result = find_state_interface_refs(
    ifaces,
    arm_kinematics::span<const NamedStateInterfaceDefinition>(defs.data(), defs.size()));

  ASSERT_TRUE(result.has_value()) << result.error().format();
  ASSERT_EQ(result->size(), 2u);
  // shoulder should come first (as declared), even though elbow is first in ifaces
  EXPECT_EQ(result->at(0).get().get_prefix_name(), "shoulder");
  EXPECT_EQ(result->at(1).get().get_prefix_name(), "elbow");
}

TEST(InterfaceRefs, StateRefs_MissingOne_ErrorListsIt)
{
  std::vector<LoanedStateInterface> ifaces;
  ifaces.push_back(make_state("shoulder", "position"));

  const std::vector<NamedStateInterfaceDefinition> defs = {
    NamedStateInterfaceDefinition{"shoulder", InterfaceId{"position"}},
    NamedStateInterfaceDefinition{"elbow", InterfaceId{"position"}},  // missing
  };

  auto result = find_state_interface_refs(
    ifaces,
    arm_kinematics::span<const NamedStateInterfaceDefinition>(defs.data(), defs.size()));

  ASSERT_FALSE(result.has_value());
  const auto * missing = std::get_if<arm_kinematics::ros2_control::InterfaceLookupError::MissingInterfaces>(
    &result.error().value);
  ASSERT_NE(missing, nullptr);
  ASSERT_EQ(missing->interface_names.size(), 1u);
  EXPECT_EQ(missing->interface_names[0], "elbow/position");
}

TEST(InterfaceRefs, StateRefs_MissingMultiple_ErrorListsAll)
{
  std::vector<LoanedStateInterface> ifaces;
  // empty — nothing to match

  const std::vector<NamedStateInterfaceDefinition> defs = {
    NamedStateInterfaceDefinition{"shoulder", InterfaceId{"position"}},
    NamedStateInterfaceDefinition{"elbow", InterfaceId{"velocity"}},
  };

  auto result = find_state_interface_refs(
    ifaces,
    arm_kinematics::span<const NamedStateInterfaceDefinition>(defs.data(), defs.size()));

  ASSERT_FALSE(result.has_value());
  const auto * missing = std::get_if<arm_kinematics::ros2_control::InterfaceLookupError::MissingInterfaces>(
    &result.error().value);
  ASSERT_NE(missing, nullptr);
  EXPECT_EQ(missing->interface_names.size(), 2u);
}

TEST(InterfaceRefs, StateRefs_WrongInterfaceType_Errors)
{
  std::vector<LoanedStateInterface> ifaces;
  ifaces.push_back(make_state("shoulder", "velocity"));  // velocity, not position

  const std::vector<NamedStateInterfaceDefinition> defs = {
    NamedStateInterfaceDefinition{"shoulder", InterfaceId{"position"}},
  };

  auto result = find_state_interface_refs(
    ifaces,
    arm_kinematics::span<const NamedStateInterfaceDefinition>(defs.data(), defs.size()));

  ASSERT_FALSE(result.has_value());
}

// ---------------------------------------------------------------------------
// find_command_interface_refs
// ---------------------------------------------------------------------------

TEST(InterfaceRefs, CommandRefs_HappyPath_OrderPreserved)
{
  std::vector<LoanedCommandInterface> ifaces;
  ifaces.push_back(make_command("elbow", "position"));    // index 0
  ifaces.push_back(make_command("shoulder", "position")); // index 1

  const std::vector<std::string> names = {
    "shoulder/position",
    "elbow/position",
  };

  auto result = find_command_interface_refs(
    ifaces,
    arm_kinematics::span<const std::string>(names.data(), names.size()));

  ASSERT_TRUE(result.has_value()) << result.error().format();
  ASSERT_EQ(result->size(), 2u);
  EXPECT_EQ(result->at(0).get().get_prefix_name(), "shoulder");
  EXPECT_EQ(result->at(1).get().get_prefix_name(), "elbow");
}

TEST(InterfaceRefs, CommandRefs_WithChainedPrefix)
{
  std::vector<LoanedCommandInterface> ifaces;
  ifaces.push_back(make_command("nova_arm_controller/j1", "position"));

  const std::vector<std::string> names = {"nova_arm_controller/j1/position"};

  auto result = find_command_interface_refs(
    ifaces,
    arm_kinematics::span<const std::string>(names.data(), names.size()));

  ASSERT_TRUE(result.has_value()) << result.error().format();
}

TEST(InterfaceRefs, CommandRefs_MissingOne_ErrorListsIt)
{
  std::vector<LoanedCommandInterface> ifaces;
  ifaces.push_back(make_command("j1", "position"));

  const std::vector<std::string> names = {"j1/position", "j2/position"};

  auto result = find_command_interface_refs(
    ifaces,
    arm_kinematics::span<const std::string>(names.data(), names.size()));

  ASSERT_FALSE(result.has_value());
  const auto * missing = std::get_if<arm_kinematics::ros2_control::InterfaceLookupError::MissingInterfaces>(
    &result.error().value);
  ASSERT_NE(missing, nullptr);
  ASSERT_EQ(missing->interface_names.size(), 1u);
  EXPECT_EQ(missing->interface_names[0], "j2/position");
}
