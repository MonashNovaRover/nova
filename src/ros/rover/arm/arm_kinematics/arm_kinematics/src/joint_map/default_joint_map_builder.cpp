//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/default_joint_map_builder.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>

#include "arm_kinematics/joint_map/joint_map_blueprint.hpp"
#include "arm_kinematics/joint_map/materialize_joint_map.hpp"
#include "arm_kinematics/joint_map/missing_input_resolution.hpp"
#include "arm_kinematics/joint_map/transmission_reachability.hpp"

namespace arm_kinematics {

namespace {

// Maximum number of interfaces to include verbatim in the formatted error message.
// Anything beyond this gets a "...and N more" suffix to keep the message bounded.
constexpr std::size_t kMaxFormattedInterfaces = 5;

// Format a StateInterfaceDefinition as "joint_name/interface_id_name" using the analysis to
// look up the joint name. Falls back to "<joint_id=N>" if the joint id is out of range.
std::string format_state_interface(
  const TransmissionAnalysis & analysis,
  const StateInterfaceDefinition & def)
{
  const auto & joint_names = analysis.joint_order().inverse;
  std::string joint_name;
  if (def.joint_id < joint_names.size()) {
    joint_name = joint_names[def.joint_id];
  } else {
    joint_name = "<joint_id=" + std::to_string(def.joint_id) + ">";
  }
  return joint_name + "/" + def.interface_id.name;
}

// Format a StateInterfaceId using the local analysis's state_interface_order inverse.
std::string format_state_interface(
  const TransmissionAnalysis & local_analysis,
  const StateInterfaceId sid)
{
  if (sid >= local_analysis.state_interface_order().inverse.size()) {
    return "<unknown sid=" + std::to_string(sid) + ">";
  }
  return format_state_interface(local_analysis, local_analysis.state_interface_order().inverse[sid]);
}

// Format a list of StateInterfaceDefinitions as "[a/position, b/velocity]", truncating after
// kMaxFormattedInterfaces with a "...and N more" suffix.
std::string format_interface_list(
  const TransmissionAnalysis & analysis,
  const std::vector<StateInterfaceDefinition> & defs)
{
  std::ostringstream oss;
  oss << "[";
  const std::size_t shown = std::min(defs.size(), kMaxFormattedInterfaces);
  for (std::size_t i = 0; i < shown; ++i) {
    if (i > 0) oss << ", ";
    oss << format_state_interface(analysis, defs[i]);
  }
  if (defs.size() > shown) {
    oss << ", ...and " << (defs.size() - shown) << " more";
  }
  oss << "]";
  return oss.str();
}

// Format a list of StateInterfaceIds via the local analysis.
std::string format_interface_list(
  const TransmissionAnalysis & local_analysis,
  const std::vector<StateInterfaceId> & sids)
{
  std::ostringstream oss;
  oss << "[";
  const std::size_t shown = std::min(sids.size(), kMaxFormattedInterfaces);
  for (std::size_t i = 0; i < shown; ++i) {
    if (i > 0) oss << ", ";
    oss << format_state_interface(local_analysis, sids[i]);
  }
  if (sids.size() > shown) {
    oss << ", ...and " << (sids.size() - shown) << " more";
  }
  oss << "]";
  return oss.str();
}

}  // namespace

tl::expected<JointMap, JointMapBuildError> DefaultJointMapBuilder::build_expected(
  const span<const StateInterfaceDefinition> inputs,
  const span<const StateInterfaceDefinition> outputs) const
{
  const std::size_t joint_count = transmission_analysis_.joint_order().inverse.size();

  // Step 0: Validate that every JointId in the request is registered in the analysis. An
  // unregistered joint_id is the caller's bug (e.g. obtained from a different analysis or
  // fabricated). Report UnknownJoint with a deduplicated list.
  std::vector<JointId> unknown_joints;
  for (const auto & def : inputs) {
    if (def.joint_id >= joint_count) {
      unknown_joints.push_back(def.joint_id);
    }
  }
  for (const auto & def : outputs) {
    if (def.joint_id >= joint_count) {
      unknown_joints.push_back(def.joint_id);
    }
  }
  if (!unknown_joints.empty()) {
    // Deduplicate while preserving insertion order.
    std::vector<JointId> unique;
    for (const auto jid : unknown_joints) {
      if (std::find(unique.begin(), unique.end(), jid) == unique.end()) {
        unique.push_back(jid);
      }
    }
    JointMapBuildError err{};
    err.kind = JointMapBuildError::Kind::UnknownJoint;
    err.unknown_joints = std::move(unique);

    std::ostringstream oss;
    oss << "DefaultJointMapBuilder: " << err.unknown_joints.size()
        << " JointId(s) in the request are not registered in the analysis: [";
    const std::size_t shown = std::min(err.unknown_joints.size(), kMaxFormattedInterfaces);
    for (std::size_t i = 0; i < shown; ++i) {
      if (i > 0) oss << ", ";
      oss << "joint_id=" << err.unknown_joints[i];
    }
    if (err.unknown_joints.size() > shown) {
      oss << ", ...and " << (err.unknown_joints.size() - shown) << " more";
    }
    oss << "]";
    err.message = oss.str();
    return tl::unexpected(std::move(err));
  }

  // Step 1: Build a local analysis copy and register every requested definition so the existing
  // StateInterfaceId-based pipeline can be used unchanged. Bare interfaces (no transmissions)
  // get an empty producing_transmissions() entry — they become pass-throughs when they also
  // appear in inputs (via producers::Input), or unproducible outputs otherwise.
  TransmissionAnalysis local = transmission_analysis_;

  std::vector<StateInterfaceId> sid_inputs;
  sid_inputs.reserve(inputs.size());
  for (const auto & def : inputs) {
    sid_inputs.push_back(local.ensure_state_interface_id(def));
  }

  std::vector<StateInterfaceId> sid_outputs;
  sid_outputs.reserve(outputs.size());
  for (const auto & def : outputs) {
    sid_outputs.push_back(local.ensure_state_interface_id(def));
  }

  // Step 2: Reachability analysis against the local analysis.
  const auto reach = TransmissionReachability::analyze(
    local, span<const StateInterfaceId>(sid_inputs.data(), sid_inputs.size()));

  // Step 3: Diagnose against the requested outputs.
  const auto diag = diagnose_missing_outputs(
    reach, span<const StateInterfaceId>(sid_outputs.data(), sid_outputs.size()));

  // Step 4: Surface errors. Ambiguity wins over MissingInputs.
  const bool has_ambiguity = !diag.directly_ambiguous_outputs.empty() ||
                             !diag.transitively_blocked_outputs.empty();
  if (has_ambiguity) {
    JointMapBuildError err{};
    err.kind = JointMapBuildError::Kind::Ambiguous;
    err.ambiguous_interfaces = std::move(diag.relevant_blocking_ambiguities);

    // Build a unified list of affected output sids (direct + transitively blocked) for the message.
    std::vector<StateInterfaceId> affected;
    affected.reserve(
      diag.directly_ambiguous_outputs.size() + diag.transitively_blocked_outputs.size());
    affected.insert(
      affected.end(),
      diag.directly_ambiguous_outputs.begin(),
      diag.directly_ambiguous_outputs.end());
    affected.insert(
      affected.end(),
      diag.transitively_blocked_outputs.begin(),
      diag.transitively_blocked_outputs.end());

    std::ostringstream oss;
    oss << "DefaultJointMapBuilder: " << affected.size()
        << " requested output(s) cannot be produced due to ambiguity";
    if (!diag.directly_ambiguous_outputs.empty() && !diag.transitively_blocked_outputs.empty()) {
      oss << " (" << diag.directly_ambiguous_outputs.size() << " directly ambiguous, "
          << diag.transitively_blocked_outputs.size()
          << " transitively blocked by upstream ambiguity)";
    } else if (!diag.transitively_blocked_outputs.empty()) {
      oss << " (transitively blocked by upstream ambiguity)";
    }
    oss << ". Affected outputs: " << format_interface_list(local, affected);
    if (!err.ambiguous_interfaces.empty()) {
      std::vector<StateInterfaceDefinition> ambiguous_defs;
      ambiguous_defs.reserve(err.ambiguous_interfaces.size());
      for (const auto & a : err.ambiguous_interfaces) {
        ambiguous_defs.push_back(a.interface);
      }
      oss << ". Upstream ambiguous interfaces: "
          << format_interface_list(local, ambiguous_defs);
    }
    err.message = oss.str();
    return tl::unexpected(std::move(err));
  }
  if (!diag.unproducible.empty()) {
    JointMapBuildError err{};
    err.kind = JointMapBuildError::Kind::MissingInputs;
    // Translate unproducible StateInterfaceIds back to StateInterfaceDefinitions.
    err.unproducible_outputs.reserve(diag.unproducible.size());
    for (const auto sid : diag.unproducible) {
      err.unproducible_outputs.push_back(local.state_interface_order().inverse[sid]);
    }
    err.resolutions = std::move(diag.resolutions);
    err.message = "DefaultJointMapBuilder: " + std::to_string(err.unproducible_outputs.size()) +
                  " requested output(s) are not derivable from the supplied inputs: " +
                  format_interface_list(local, err.unproducible_outputs);
    return tl::unexpected(std::move(err));
  }

  // Step 5: Plan and materialize against the local analysis.
  const auto blueprint = plan_joint_map(
    reach, span<const StateInterfaceId>(sid_outputs.data(), sid_outputs.size()));
  return materialize_joint_map(blueprint, local);
}

}  // namespace arm_kinematics
