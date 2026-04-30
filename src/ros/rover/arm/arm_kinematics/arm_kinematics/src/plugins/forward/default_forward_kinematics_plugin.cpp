//
// Created by Bailey Chessum on 17/11/2025.
//

#include "../../../include/arm_kinematics/plugins/forward/default_forward_kinematics_plugin.hpp"
#include "arm_kinematics/forward/utilities/compute_frame_tree.hpp"
#include "arm_kinematics/common/robot_model.hpp"
#include "arm_kinematics/utilities/interface_id.hpp"
#include "arm_kinematics/utilities/param_reader.hpp"

#include <cctype>
#include <exception>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <rclcpp/logging.hpp>

namespace arm_kinematics {

static_assert(std::is_base_of_v<
  ForwardKinematicsPlugin::Tree,
  DefaultForwardKinematicsPlugin::TreeImpl>);

const JointMapBuilder & DefaultForwardKinematicsPlugin::get_joint_map_builder() const noexcept
{
  // One-shot lazy init. The captured analysis reference is valid for as long as
  // `get_transmission_analysis()` returns a stable object — which it does for the default
  // RobotModel-backed implementation. Subclasses that override `get_transmission_analysis()`
  // to return a different object on different calls would need to override this method too.
  std::call_once(joint_map_builder_once_, [this]() {
    joint_map_builder_ = std::make_unique<DefaultJointMapBuilder>(
      get_transmission_analysis(),
      default_state_interface_values_);
  });
  return *joint_map_builder_;
}

tl::expected<ForwardKinematicsPlugin::MakeTreeResult, ForwardKinematicsPlugin::MakeTreeError>
DefaultForwardKinematicsPlugin::make_tree(
  const span<const StateInterfaceDefinition> input_state_interfaces,
  const std::string & base_link_name,
  const FrameDefinitions & frames,
  const JointMapBuilder & joint_map_builder)
{
  AnalysisTree subtree(get_robot_model().get_analysis_tree(), base_link_name, frames);

  // Joints need to be in the right order to be able to construct a compute frame tree.
  subtree.sort_joints();
  // The mapper needs joint POSITION values in the order produced by the sorted analysis
  // subtree (skipping index 0, which is the base link). Resolve joint names to JointIds via
  // the FK plugin's analysis and pair them with the position interface.
  const span<const std::string> all_joint_names = subtree.joint_names();
  const auto & analysis = get_transmission_analysis();
  const auto & joint_order = analysis.joint_order();
  static const InterfaceId k_position_interface{"position"};

  // Skip index 0 — that's the base link, not a joint with state.
  const span<const std::string> joint_names_to_resolve{
    all_joint_names.data() + 1,
    all_joint_names.size() > 0 ? all_joint_names.size() - 1 : std::size_t{0}};

  auto joint_id_result = joint_order.try_map_collect(joint_names_to_resolve);

  if (!joint_id_result) {
    // The FK subtree references joints that aren't in the FK plugin's analysis. This is an
    // internal inconsistency between the URDF (which feeds the analysis) and the analysis
    // tree (which feeds the FK subtree).
    const auto & unknown = joint_id_result.error();
    return tl::unexpected(MakeTreeError::UnknownJoint{unknown});
  }

  // Build the position-interface StateInterfaceDefinitions from the resolved JointIds.
  std::vector<StateInterfaceDefinition> mapper_output_defs;
  mapper_output_defs.reserve(joint_id_result->size());
  for (const JointId jid : *joint_id_result) {
    mapper_output_defs.push_back(StateInterfaceDefinition{jid, k_position_interface});
  }

  // Sort frames such that any root-relative frames are placed at the end of the array.
  auto frame_order = subtree.sort_frames();

  // Create the compute tree. A failure here is a real error (bad URDF parent reference,
  // unresolved frame, etc.) — surface it as a MakeTreeError::FrameTreeFailed instead of
  // silently substituting an empty tree.
  auto compute_frame_tree = subtree.make_compute_frame_tree();
  if (!compute_frame_tree.has_value()) {
    return tl::unexpected(MakeTreeError::FrameTreeFailed{std::string{compute_frame_tree.error()}});
  }

  // Build the runtime joint map via the builder API.
  auto joint_map_result = joint_map_builder.build_expected(
    input_state_interfaces,
    span<const StateInterfaceDefinition>(mapper_output_defs.data(), mapper_output_defs.size()));
  if (!joint_map_result.has_value()) {
    return tl::unexpected(MakeTreeError::JointMapBuildFailed{std::move(joint_map_result.error())});
  }

  auto ptr = std::make_unique<TreeImpl>(
    frames.size(),
    std::move(compute_frame_tree.value()),
    std::move(joint_map_result.value()));

  return MakeTreeResult{
    std::move(ptr),
    std::move(frame_order)
  };
}

bool DefaultForwardKinematicsPlugin::on_initialize() {
  const ParamReader params(
    get_node_interfaces().get<rclcpp::node_interfaces::NodeParametersInterface>());

  const auto entries = params.get<std::vector<std::string>>(
    "kinematics.default_state_interface_values", {});

  if (entries.empty()) {
    return true;
  }

  const auto & analysis = get_transmission_analysis();
  const auto & joint_order = analysis.joint_order();

  // Trim leading/trailing whitespace from a string_view.
  const auto trim = [](std::string_view sv) -> std::string_view {
    std::size_t s = 0;
    while (s < sv.size() && std::isspace(static_cast<unsigned char>(sv[s]))) ++s;
    std::size_t e = sv.size();
    while (e > s && std::isspace(static_cast<unsigned char>(sv[e - 1]))) --e;
    return sv.substr(s, e - s);
  };

  for (const auto & entry : entries) {
    // Expected format: "joint_name/interface_name=value"
    // Joint names may contain '/', so split on the last '/' before the '='.
    const auto eq = entry.find('=');
    if (eq == std::string::npos) {
      RCLCPP_WARN(
        get_logger(),
        "Skipping malformed kinematics.default_state_interface_values entry '%s'."
        " Expected 'joint/interface=value'.",
        entry.c_str());
      continue;
    }

    const std::string_view lhs = trim(std::string_view(entry).substr(0, eq));
    const std::string_view value_sv = trim(std::string_view(entry).substr(eq + 1));

    const auto slash = lhs.rfind('/');
    if (slash == std::string_view::npos || slash == 0 || slash + 1 == lhs.size()) {
      RCLCPP_WARN(
        get_logger(),
        "Skipping malformed kinematics.default_state_interface_values entry '%s'."
        " Expected 'joint/interface=value'.",
        entry.c_str());
      continue;
    }

    const std::string joint_name{lhs.substr(0, slash)};
    const std::string interface_name{lhs.substr(slash + 1)};

    if (joint_name.empty() || interface_name.empty() || value_sv.empty()) {
      RCLCPP_WARN(
        get_logger(),
        "Skipping malformed kinematics.default_state_interface_values entry '%s'.",
        entry.c_str());
      continue;
    }

    if (!joint_order.contains_key(joint_name)) {
      RCLCPP_WARN(
        get_logger(),
        "Skipping unknown joint '%s' in kinematics.default_state_interface_values entry '%s'.",
        joint_name.c_str(),
        entry.c_str());
      continue;
    }

    try {
      const std::string value_str{value_sv};
      std::size_t parsed_chars = 0;
      const double value = std::stod(value_str, &parsed_chars);
      if (parsed_chars != value_str.size()) {
        throw std::invalid_argument("trailing characters");
      }
      default_state_interface_values_[{joint_order[joint_name], InterfaceId{interface_name}}] =
        value;
    } catch (const std::exception &) {
      RCLCPP_WARN(
        get_logger(),
        "Skipping kinematics.default_state_interface_values entry '%s':"
        " value '%s' is not a valid number.",
        entry.c_str(),
        std::string{value_sv}.c_str());
    }
  }

  return true;
}

} // arm_kinematics

#include <pluginlib/class_list_macros.hpp>

PLUGINLIB_EXPORT_CLASS(arm_kinematics::DefaultForwardKinematicsPlugin, arm_kinematics::ForwardKinematicsPlugin)
