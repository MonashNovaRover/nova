//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/default_joint_map_builder.hpp"

#include <sstream>
#include <string>
#include <utility>

#include "arm_kinematics/joint_map/joint_map_blueprint.hpp"
#include "arm_kinematics/joint_map/materialize_joint_map.hpp"
#include "arm_kinematics/joint_map/missing_input_resolution.hpp"
#include "arm_kinematics/joint_map/transmission_reachability.hpp"

namespace arm_kinematics {

namespace {

// Maximum number of state interfaces to include verbatim in the formatted error message.
// Anything beyond this gets a "...and N more" suffix to keep the message bounded.
constexpr std::size_t kMaxFormattedInterfaces = 5;

// Format a single StateInterfaceId as "joint_name.interface_id_name (sid=N)" using the analysis
// to look up the joint and interface names. Falls back to "<unknown sid=N>" if the id is out
// of range.
std::string format_state_interface(
  const TransmissionAnalysis & analysis,
  const StateInterfaceId sid)
{
  const auto & defs = analysis.state_interface_order().inverse;
  if (sid >= defs.size()) {
    return "<unknown sid=" + std::to_string(sid) + ">";
  }
  const auto & def = defs[sid];
  const auto & joint_names = analysis.joint_order().inverse;
  std::string joint_name;
  if (def.joint_id < joint_names.size()) {
    joint_name = joint_names[def.joint_id];
  } else {
    joint_name = "<joint_id=" + std::to_string(def.joint_id) + ">";
  }
  return joint_name + "." + def.interface_id.name + " (sid=" + std::to_string(sid) + ")";
}

// Format a list of StateInterfaceIds as `[a.position (sid=1), b.velocity (sid=2)]`, truncating
// after kMaxFormattedInterfaces with a "...and N more" suffix.
std::string format_interface_list(
  const TransmissionAnalysis & analysis,
  const std::vector<StateInterfaceId> & sids)
{
  std::ostringstream oss;
  oss << "[";
  const std::size_t shown = std::min(sids.size(), kMaxFormattedInterfaces);
  for (std::size_t i = 0; i < shown; ++i) {
    if (i > 0) oss << ", ";
    oss << format_state_interface(analysis, sids[i]);
  }
  if (sids.size() > shown) {
    oss << ", ...and " << (sids.size() - shown) << " more";
  }
  oss << "]";
  return oss.str();
}

// Validate that every StateInterfaceId in `ids` is in range for the analysis. Returns the
// out-of-range ids (empty if all are valid).
std::vector<StateInterfaceId> find_unknown_ids(
  const TransmissionAnalysis & analysis,
  const span<const StateInterfaceId> ids)
{
  const std::size_t state_count = analysis.state_interface_order().inverse.size();
  std::vector<StateInterfaceId> unknown;
  for (const auto sid : ids) {
    if (sid >= state_count) {
      unknown.push_back(sid);
    }
  }
  return unknown;
}

}  // namespace

tl::expected<JointMap, JointMapBuildError> DefaultJointMapBuilder::build_expected(
  const span<const StateInterfaceId> inputs,
  const span<const StateInterfaceId> outputs) const
{
  // Step 0: Validate that every requested id is known to the analysis. Catches stale or
  // fabricated StateInterfaceIds early so the user gets a clear error instead of a misleading
  // "unreachable" report.
  auto unknown = find_unknown_ids(transmission_analysis_, inputs);
  auto unknown_outputs = find_unknown_ids(transmission_analysis_, outputs);
  unknown.insert(unknown.end(), unknown_outputs.begin(), unknown_outputs.end());
  if (!unknown.empty()) {
    JointMapBuildError err{};
    err.kind = JointMapBuildError::Kind::UnknownInterface;
    err.unknown_interfaces = std::move(unknown);
    err.message = "DefaultJointMapBuilder: " + std::to_string(err.unknown_interfaces.size()) +
                  " StateInterfaceId(s) in the request are unknown to the analysis: " +
                  format_interface_list(transmission_analysis_, err.unknown_interfaces);
    return tl::unexpected(std::move(err));
  }

  // Step 1: Reachability analysis. Output-independent — purely a function of the analysis +
  // inputs.
  auto reach = TransmissionReachability::analyze(transmission_analysis_, inputs);

  // Step 2: Diagnose against the requested outputs. Catches:
  //   - directly unreachable outputs (no producer)
  //   - directly ambiguous outputs (≥2 candidate producers)
  //   - blocked outputs (producer chain depends on an upstream ambiguous interface)
  auto diag = diagnose_missing_outputs(
    reach, span<const StateInterfaceId>(outputs.data_, outputs.size_));

  // Step 3: If anything is broken, surface it. Ambiguity (direct or blocked) wins over
  // MissingInputs.
  const bool has_ambiguity = !diag.directly_ambiguous_outputs.empty() ||
                             !diag.blocked_outputs.empty();
  if (has_ambiguity) {
    JointMapBuildError err{};
    err.kind = JointMapBuildError::Kind::Ambiguous;
    err.ambiguous_interfaces = std::move(diag.relevant_blocking_ambiguities);

    // Build a unified list of affected outputs (direct + blocked) for the message.
    std::vector<StateInterfaceId> affected;
    affected.reserve(diag.directly_ambiguous_outputs.size() + diag.blocked_outputs.size());
    affected.insert(
      affected.end(),
      diag.directly_ambiguous_outputs.begin(),
      diag.directly_ambiguous_outputs.end());
    affected.insert(
      affected.end(),
      diag.blocked_outputs.begin(),
      diag.blocked_outputs.end());

    std::ostringstream oss;
    oss << "DefaultJointMapBuilder: " << affected.size()
        << " requested output(s) cannot be produced due to ambiguity";
    if (!diag.directly_ambiguous_outputs.empty() && !diag.blocked_outputs.empty()) {
      oss << " (" << diag.directly_ambiguous_outputs.size() << " directly ambiguous, "
          << diag.blocked_outputs.size() << " blocked by upstream ambiguity)";
    } else if (!diag.blocked_outputs.empty()) {
      oss << " (blocked by upstream ambiguity)";
    }
    oss << ". Affected outputs: " << format_interface_list(transmission_analysis_, affected);
    if (!err.ambiguous_interfaces.empty()) {
      std::vector<StateInterfaceId> ambiguous_ids;
      ambiguous_ids.reserve(err.ambiguous_interfaces.size());
      for (const auto & a : err.ambiguous_interfaces) {
        ambiguous_ids.push_back(a.interface);
      }
      oss << ". Upstream ambiguous interfaces: "
          << format_interface_list(transmission_analysis_, ambiguous_ids);
    }
    err.message = oss.str();
    return tl::unexpected(std::move(err));
  }
  if (!diag.unreachable.empty()) {
    JointMapBuildError err{};
    err.kind = JointMapBuildError::Kind::MissingInputs;
    err.unreachable_outputs = std::move(diag.unreachable);
    err.resolutions = std::move(diag.resolutions);
    err.message = "DefaultJointMapBuilder: " + std::to_string(err.unreachable_outputs.size()) +
                  " requested output(s) are not derivable from the supplied inputs: " +
                  format_interface_list(transmission_analysis_, err.unreachable_outputs);
    return tl::unexpected(std::move(err));
  }

  // Step 4: Plan and materialize.
  const auto blueprint = plan_joint_map(
    reach, span<const StateInterfaceId>(outputs.data_, outputs.size_));
  return materialize_joint_map(blueprint, transmission_analysis_);
}

}  // namespace arm_kinematics
