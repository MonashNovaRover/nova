//
// Created by Bailey Chessum on 17/11/2025.
//

#include "../../../include/arm_kinematics/plugins/forward/eigen_forward_kinematics_plugin.hpp"
#include "arm_kinematics/forward/utilities/compute_frame_tree.hpp"

#include <sstream>
#include <utility>
#include <vector>

namespace arm_kinematics {

static_assert(std::is_base_of_v<
  ForwardKinematicsPlugin::Tree,
  EigenForwardKinematicsPlugin::TreeImpl>);

tl::expected<ForwardKinematicsPlugin::MakeTreeResult, JointMapBuildError>
EigenForwardKinematicsPlugin::make_tree(
  const span<const StateInterfaceId> input_state_interfaces,
  const std::string & base_link_name,
  const FrameDefinitions & frames,
  const JointMapBuilder & joint_map_builder)
{
  AnalysisTree subtree(get_robot_model().get_analysis_tree(), base_link_name, frames);

  // Joints need to be in the right order to be able to construct a compute frame tree.
  subtree.sort_joints();
  // The mapper needs joint POSITION values in the order produced by the sorted analysis
  // subtree (skipping index 0, which is the base link). These are joint names; we resolve
  // them to position-interface StateInterfaceIds against the FK plugin's analysis (the
  // analysis is logically frozen post-URDF-parse, so this is a const lookup).
  const auto & subtree_joint_names = subtree.get_joints().names;
  const auto & analysis = get_transmission_analysis();
  const auto & joint_order = analysis.joint_order();
  const auto & state_interface_order = analysis.state_interface_order();
  static const InterfaceId k_position_interface{"position"};

  std::vector<StateInterfaceId> mapper_output_sids;
  mapper_output_sids.reserve(subtree_joint_names.size());
  std::vector<std::string> unknown_mapper_joints;

  // Skip index 0 — that's the base link, not a joint with state.
  for (std::size_t i = 1; i < subtree_joint_names.size(); ++i) {
    const std::string & joint_name = subtree_joint_names[i];
    if (!joint_order.contains_key(joint_name)) {
      unknown_mapper_joints.push_back(joint_name);
      continue;
    }
    const JointId joint_id = joint_order[joint_name];
    const StateInterfaceDefinition def{joint_id, k_position_interface};
    if (!state_interface_order.contains_key(def)) {
      unknown_mapper_joints.push_back(joint_name);
      continue;
    }
    mapper_output_sids.push_back(state_interface_order[def]);
  }

  if (!unknown_mapper_joints.empty()) {
    // The FK subtree references joints that aren't in the FK plugin's analysis. This is an
    // internal inconsistency between the URDF (which feeds the analysis) and the analysis
    // tree (which feeds the FK subtree). Surface as UnknownInterface so the user can
    // diagnose.
    JointMapBuildError err{};
    err.kind = JointMapBuildError::Kind::UnknownInterface;
    std::ostringstream oss;
    oss << "EigenForwardKinematicsPlugin::make_tree: " << unknown_mapper_joints.size()
        << " joint(s) referenced by the FK analysis subtree are not registered in the FK "
        << "plugin's transmission analysis: [";
    constexpr std::size_t kMaxFormatted = 5;
    const std::size_t shown = std::min(unknown_mapper_joints.size(), kMaxFormatted);
    for (std::size_t i = 0; i < shown; ++i) {
      if (i > 0) oss << ", ";
      oss << unknown_mapper_joints[i];
    }
    if (unknown_mapper_joints.size() > shown) {
      oss << ", ...and " << (unknown_mapper_joints.size() - shown) << " more";
    }
    oss << "]";
    err.message = oss.str();
    return tl::unexpected(std::move(err));
  }

  // Sort frames such that any root-relative frames are placed at the end of the array.
  auto frame_order = subtree.sort_frames();

  // Create the compute tree.
  auto compute_frame_tree = subtree.make_compute_frame_tree();
  if (!compute_frame_tree.has_value()) {
    RCLCPP_ERROR(get_logger(), "Failed to make FK Tree: %s", compute_frame_tree.error().data());
    compute_frame_tree = ComputeFrameTree();  // Use an empty tree that will do nothing.
  }

  // Build the runtime joint map via the new builder API.
  auto joint_map_result = joint_map_builder.build_expected(
    input_state_interfaces,
    span<const StateInterfaceId>(mapper_output_sids.data(), mapper_output_sids.size()));
  if (!joint_map_result.has_value()) {
    return tl::unexpected(std::move(joint_map_result.error()));
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

bool EigenForwardKinematicsPlugin::on_initialize() {
  // TODO: Could we precompute joints we wouldn't have the values for here?

  return true;
}

} // arm_kinematics

#include <pluginlib/class_list_macros.hpp>

PLUGINLIB_EXPORT_CLASS(arm_kinematics::EigenForwardKinematicsPlugin, arm_kinematics::ForwardKinematicsPlugin)
