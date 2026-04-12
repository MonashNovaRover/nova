//
// Created by Bailey Chessum on 14/10/2025.
//

#include "arm_kinematics/forward/forward_kinematics_plugin.hpp"

#include <optional>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include <urdf/model.h>

#include "arm_kinematics/common/robot_model.hpp"

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
    std::ostringstream oss;
    oss << "ForwardKinematicsPlugin::make_tree: " << unknown.size()
        << " requested state interface(s) reference joint names not registered in the FK "
        << "plugin's analysis (joint name not in URDF): [";
    constexpr std::size_t kMaxFormatted = 5;
    const std::size_t shown = std::min(unknown.size(), kMaxFormatted);
    for (std::size_t i = 0; i < shown; ++i) {
      if (i > 0) oss << ", ";
      oss << unknown[i].joint_name << "/" << unknown[i].interface_id.name;
    }
    if (unknown.size() > shown) {
      oss << ", ...and " << (unknown.size() - shown) << " more";
    }
    oss << "]";
    JointMapBuildError joint_map_err{};
    joint_map_err.kind = JointMapBuildError::Kind::UnknownJoint;
    joint_map_err.message = oss.str();
    MakeTreeError err{};
    err.kind = MakeTreeError::Kind::UnknownInterface;
    err.message = joint_map_err.message;
    err.joint_map_error = std::move(joint_map_err);
    return tl::unexpected(std::move(err));
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
