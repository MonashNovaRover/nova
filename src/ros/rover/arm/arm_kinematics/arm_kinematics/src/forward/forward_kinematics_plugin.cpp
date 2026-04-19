//
// Created by Bailey Chessum on 14/10/2025.
//

#include "arm_kinematics/forward/forward_kinematics_plugin.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

#include <urdf/model.h>

#include "arm_kinematics/common/robot_model.hpp"

namespace arm_kinematics {

std::string ForwardKinematicsPlugin::MakeTreeError::JointMapBuildFailed::format() const
{
  return "ForwardKinematicsPlugin::make_tree: joint map builder rejected the request: " +
         error.format();
}

std::string ForwardKinematicsPlugin::MakeTreeError::FrameTreeFailed::format() const
{
  return "ForwardKinematicsPlugin::make_tree: failed to build compute frame tree: " + detail;
}

std::string ForwardKinematicsPlugin::MakeTreeError::UnknownJoint::format() const
{
  std::ostringstream oss;
  oss << "ForwardKinematicsPlugin::make_tree: " << unknown_joint_names.size()
      << " requested joint name(s) are not registered in the FK plugin analysis: [";
  constexpr std::size_t kMaxFormatted = 5;
  const std::size_t shown = std::min(unknown_joint_names.size(), kMaxFormatted);
  for (std::size_t i = 0; i < shown; ++i) {
    if (i > 0) {
      oss << ", ";
    }
    oss << unknown_joint_names[i];
  }
  if (unknown_joint_names.size() > shown) {
    oss << ", ...and " << (unknown_joint_names.size() - shown) << " more";
  }
  oss << "]";
  return oss.str();
}

std::string ForwardKinematicsPlugin::MakeTreeError::format() const
{
  return std::visit([](const auto & error) { return error.format(); }, value);
}

bool ForwardKinematicsPlugin::initialize(
  const KinematicsNodeInterfaces & node_interfaces,
  const RobotModel & robot_model,
  KinematicsParams::SharedPtr kinematics_params)
{
  if (!initialize_base(node_interfaces, robot_model, std::move(kinematics_params), "forward_kinematics"))
    return false;

  // Set up URDF
  RCLCPP_INFO(get_logger(), "Parsing URDF and creating KDL Tree...");

  return on_initialize();
}

const TransmissionAnalysis & ForwardKinematicsPlugin::get_transmission_analysis() const noexcept
{
  return get_robot_model().get_default_transmission_analysis();
}

tl::expected<ForwardKinematicsPlugin::MakeTreeResult, ForwardKinematicsPlugin::MakeTreeError>
ForwardKinematicsPlugin::make_tree(
  const span<const NamedStateInterfaceDefinition> named_input_state_interfaces,
  const std::string & base_link_name,
  const FrameDefinitions & frames,
  const JointMapBuilder & joint_map_builder)
{
  // Resolve each (joint_name, interface_id) to a StateInterfaceDefinition by looking up the
  // joint name in the FK plugin's analysis. Any unknown joint name is surfaced as UnknownJoint
  // so the caller knows their request references a joint not registered in the URDF.
  const auto & analysis = get_transmission_analysis();
  const auto & joint_order = analysis.joint_order();

  auto resolved = joint_order.try_map_collect(
    named_input_state_interfaces,
    [](const NamedStateInterfaceDefinition & named) -> std::optional<std::string> {
      return named.joint_name;
    });

  if (!resolved) {
    const auto & unknown = resolved.error();
    std::vector<std::string> unknown_joint_names;
    unknown_joint_names.reserve(unknown.size());
    for (const auto & entry : unknown) {
      unknown_joint_names.push_back(entry.joint_name + "/" + entry.interface_id.name);
    }
    return tl::unexpected(MakeTreeError::UnknownJoint{std::move(unknown_joint_names)});
  }

  // Build StateInterfaceDefinitions from the resolved JointIds + the original interface ids.
  std::vector<StateInterfaceDefinition> defs;
  defs.reserve(named_input_state_interfaces.size());
  for (std::size_t i = 0; i < named_input_state_interfaces.size(); ++i) {
    defs.push_back(StateInterfaceDefinition{(*resolved)[i], named_input_state_interfaces[i].interface_id});
  }

  return make_tree(
    span<const StateInterfaceDefinition>(defs.data(), defs.size()),
    base_link_name,
    frames,
    joint_map_builder);
}

} // arm_kinematics
