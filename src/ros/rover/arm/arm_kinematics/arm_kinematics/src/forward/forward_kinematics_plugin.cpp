//
// Created by Bailey Chessum on 14/10/2025.
//

#include "arm_kinematics/forward/forward_kinematics_plugin.hpp"

#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include <urdf/model.h>

namespace arm_kinematics {

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

tl::expected<ForwardKinematicsPlugin::MakeTreeResult, JointMapBuildError>
ForwardKinematicsPlugin::make_tree(
  const span<const NamedStateInterfaceDefinition> named_input_state_interfaces,
  const std::string & base_link_name,
  const FrameDefinitions & frames,
  const JointMapBuilder & joint_map_builder)
{
  // Const lookup of each (joint_name, interface_id) against the FK plugin's analysis. The
  // analysis is logically frozen at this point — typically populated by URDF parsing in
  // RobotModel — so we use `contains_key` + `operator[]` instead of mutating helpers like
  // `ensure_state_interface_id`. If any name/interface is unknown, return a structured
  // UnknownInterface error so the caller knows their request references something not in
  // the analysis.
  const auto & analysis = get_transmission_analysis();
  const auto & joint_order = analysis.joint_order();
  const auto & state_interface_order = analysis.state_interface_order();

  std::vector<StateInterfaceId> resolved;
  resolved.reserve(named_input_state_interfaces.size());

  std::vector<NamedStateInterfaceDefinition> unknown;

  for (const auto & named : named_input_state_interfaces) {
    if (!joint_order.contains_key(named.joint_name)) {
      unknown.push_back(named);
      continue;
    }
    const JointId joint_id = joint_order[named.joint_name];
    const StateInterfaceDefinition def{joint_id, named.interface_id};
    if (!state_interface_order.contains_key(def)) {
      unknown.push_back(named);
      continue;
    }
    resolved.push_back(state_interface_order[def]);
  }

  if (!unknown.empty()) {
    JointMapBuildError err{};
    err.kind = JointMapBuildError::Kind::UnknownInterface;
    std::ostringstream oss;
    oss << "ForwardKinematicsPlugin::make_tree: " << unknown.size()
        << " requested state interface(s) are not registered in the FK plugin's analysis "
        << "(joint name not in URDF, or interface id not present for that joint): [";
    constexpr std::size_t kMaxFormatted = 5;
    const std::size_t shown = std::min(unknown.size(), kMaxFormatted);
    for (std::size_t i = 0; i < shown; ++i) {
      if (i > 0) oss << ", ";
      oss << unknown[i].joint_name << "." << unknown[i].interface_id.name;
    }
    if (unknown.size() > shown) {
      oss << ", ...and " << (unknown.size() - shown) << " more";
    }
    oss << "]";
    err.message = oss.str();
    return tl::unexpected(std::move(err));
  }

  return make_tree(
    span<const StateInterfaceId>(resolved.data(), resolved.size()),
    base_link_name,
    frames,
    joint_map_builder);
}

} // arm_kinematics
