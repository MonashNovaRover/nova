//
// Created by Bailey Chessum on 17/11/2025.
//

#include "../../../include/arm_kinematics/plugins/forward/default_forward_kinematics_plugin.hpp"
#include "arm_kinematics/forward/utilities/compute_frame_tree.hpp"
#include "arm_kinematics/common/robot_model.hpp"

#include <optional>
#include <sstream>
#include <utility>
#include <vector>

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
    joint_map_builder_ = std::make_unique<DefaultJointMapBuilder>(get_transmission_analysis());
  });
  return *joint_map_builder_;
}

tl::expected<ForwardKinematicsPlugin::MakeTreeResult, ForwardKinematicsPlugin::MakeTreeError>
DefaultForwardKinematicsPlugin::make_tree(
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

  // Skip index 0 — that's the base link, not a joint with state.
  const span<const std::string> joint_names_to_resolve{
    subtree_joint_names.data() + 1,
    subtree_joint_names.size() > 0 ? subtree_joint_names.size() - 1 : std::size_t{0}};

  auto mapper_result = state_interface_order.try_map_collect(
    joint_names_to_resolve,
    [&joint_order](const std::string & name) -> std::optional<StateInterfaceDefinition> {
      if (!joint_order.contains_key(name)) return std::nullopt;
      return StateInterfaceDefinition{joint_order[name], k_position_interface};
    });

  if (!mapper_result) {
    // The FK subtree references joints that aren't in the FK plugin's analysis. This is an
    // internal inconsistency between the URDF (which feeds the analysis) and the analysis
    // tree (which feeds the FK subtree). Surface as UnknownInterface so the user can
    // diagnose.
    const auto & unknown = mapper_result.error();
    std::ostringstream oss;
    oss << "DefaultForwardKinematicsPlugin::make_tree: " << unknown.size()
        << " joint(s) referenced by the FK analysis subtree are not registered in the FK "
        << "plugin's transmission analysis: [";
    constexpr std::size_t kMaxFormatted = 5;
    const std::size_t shown = std::min(unknown.size(), kMaxFormatted);
    for (std::size_t i = 0; i < shown; ++i) {
      if (i > 0) oss << ", ";
      oss << unknown[i];
    }
    if (unknown.size() > shown) {
      oss << ", ...and " << (unknown.size() - shown) << " more";
    }
    oss << "]";
    JointMapBuildError joint_map_err{};
    joint_map_err.kind = JointMapBuildError::Kind::UnknownInterface;
    joint_map_err.message = oss.str();
    MakeTreeError err{};
    err.kind = MakeTreeError::Kind::UnknownInterface;
    err.message = joint_map_err.message;
    err.joint_map_error = std::move(joint_map_err);
    return tl::unexpected(std::move(err));
  }
  auto & mapper_output_sids = *mapper_result;

  // Sort frames such that any root-relative frames are placed at the end of the array.
  auto frame_order = subtree.sort_frames();

  // Create the compute tree. A failure here is a real error (bad URDF parent reference,
  // unresolved frame, etc.) — surface it as a MakeTreeError::FrameTreeFailed instead of
  // silently substituting an empty tree.
  auto compute_frame_tree = subtree.make_compute_frame_tree();
  if (!compute_frame_tree.has_value()) {
    MakeTreeError err{};
    err.kind = MakeTreeError::Kind::FrameTreeFailed;
    err.message = std::string{"DefaultForwardKinematicsPlugin::make_tree: failed to build "
                              "compute frame tree: "} + std::string{compute_frame_tree.error()};
    return tl::unexpected(std::move(err));
  }

  // Build the runtime joint map via the builder API.
  auto joint_map_result = joint_map_builder.build_expected(
    input_state_interfaces,
    span<const StateInterfaceId>(mapper_output_sids.data(), mapper_output_sids.size()));
  if (!joint_map_result.has_value()) {
    MakeTreeError err{};
    err.kind = MakeTreeError::Kind::JointMapBuildFailed;
    err.message = "DefaultForwardKinematicsPlugin::make_tree: joint map builder rejected the "
                  "request: " + joint_map_result.error().message;
    err.joint_map_error = std::move(joint_map_result.error());
    return tl::unexpected(std::move(err));
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
  // TODO: Could we precompute joints we wouldn't have the values for here?

  return true;
}

} // arm_kinematics

#include <pluginlib/class_list_macros.hpp>

PLUGINLIB_EXPORT_CLASS(arm_kinematics::DefaultForwardKinematicsPlugin, arm_kinematics::ForwardKinematicsPlugin)
