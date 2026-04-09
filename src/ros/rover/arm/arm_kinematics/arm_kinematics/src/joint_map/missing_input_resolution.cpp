//
// Created by Bailey Chessum on 8/4/26.
//

#include "arm_kinematics/joint_map/missing_input_resolution.hpp"

#include <algorithm>
#include <unordered_set>

#include "arm_kinematics/joint_map/transmission_reachability.hpp"

namespace arm_kinematics {

namespace {

// Walk the analysis's potential-producer graph backward from `blocked_output` and collect
// every ambiguous-interface ancestor (transitively, through transmission inputs AND affine
// projection sources). Used only on the failing path to attribute "this output is blocked
// because of these upstream ambiguities".
//
// `ambiguous_set` is the precomputed lookup of every ambiguous interface in the reachability,
// for O(1) checks.
//
// The walk is bounded by the number of state interfaces (visited set prevents revisits).
// Cost: O(reachable potential-producer subgraph + affine-group-size sum), only on the
// failing path.
void collect_blocking_ambiguities(
  const TransmissionReachability & reach,
  const StateInterfaceId blocked_output,
  const std::unordered_set<StateInterfaceId> & ambiguous_set,
  std::unordered_set<StateInterfaceId> & visited,
  std::unordered_set<StateInterfaceId> & blocking_ambiguities)
{
  if (!visited.insert(blocked_output).second) {
    return;
  }
  if (ambiguous_set.count(blocked_output) > 0) {
    blocking_ambiguities.insert(blocked_output);
    // Don't early-return — keep walking to find any deeper upstream ambiguities. The user
    // benefits from seeing the FULL transitive closure of related ambiguities so they can
    // fix everything in one pass instead of round-tripping through multiple build attempts.
    // The visited set prevents infinite recursion.
  }

  // ---- Walk producing transmissions (and recurse on each transmission's inputs) ----
  const auto & analysis = reach.analysis();
  const auto producing = analysis.producing_transmissions(blocked_output);
  for (const auto tid : producing) {
    const auto & instance = analysis.transmissions()[tid];
    for (const auto in : instance.input_ids) {
      collect_blocking_ambiguities(reach, in, ambiguous_set, visited, blocking_ambiguities);
    }
  }

  // ---- Walk affine group members (for projection-driven blocking) ----
  // If this interface lives in a non-trivial affine group with a registered projection rule,
  // any other group member at the same interface_id is a potential affine source. Recurse
  // into each, so a blocked output that's only producible via affine projection from an
  // ambiguous source still gets its upstream cause attributed.
  const auto & defn = analysis.state_interface_order().inverse[blocked_output];
  const AffineProjectionRule * rule = analysis.affine_projection_rule(defn.interface_id);
  if (rule != nullptr) {
    const JointId root = analysis.affine_root_of(defn.joint_id);
    const auto group_members = analysis.affine_group_members(root);
    if (group_members.size() > 1) {
      for (const JointId member_joint : group_members) {
        if (member_joint == defn.joint_id) continue;
        const auto member_def = StateInterfaceDefinition{member_joint, defn.interface_id};
        if (!analysis.state_interface_order().contains_key(member_def)) continue;
        const StateInterfaceId member_sid = analysis.state_interface_order()[member_def];
        collect_blocking_ambiguities(reach, member_sid, ambiguous_set, visited, blocking_ambiguities);
      }
    }
  }
}

}  // namespace

MissingOutputDiagnosis diagnose_missing_outputs(
  const TransmissionReachability & reach,
  const span<const StateInterfaceId> needed_outputs)
{
  MissingOutputDiagnosis diag{};

  const auto ambiguities = reach.ambiguities();
  std::unordered_set<StateInterfaceId> ambiguous_set;
  ambiguous_set.reserve(ambiguities.size());
  for (const auto & amb : ambiguities) {
    ambiguous_set.insert(amb.interface);
  }

  // Lookup for blocked classification — turn the span into a set once.
  const auto blocked_span = reach.blocked_interfaces();
  std::unordered_set<StateInterfaceId> blocked_set;
  blocked_set.reserve(blocked_span.size());
  for (const auto sid : blocked_span) {
    blocked_set.insert(sid);
  }

  std::unordered_set<StateInterfaceId> unreachable_seen;
  std::unordered_set<StateInterfaceId> ambiguous_seen;
  std::unordered_set<StateInterfaceId> blocking_ambiguities;

  for (const StateInterfaceId out : needed_outputs) {
    const auto producer = reach.producer_of(out);
    if (!std::holds_alternative<std::monostate>(producer)) {
      continue;  // satisfied
    }
    if (ambiguous_set.count(out) > 0 || blocked_set.count(out) > 0) {
      // Directly ambiguous OR blocked: in either case, the output is unbuildable due to
      // ambiguity (direct or transitive). Walk the producer chain to attribute the FULL
      // transitive closure of related ambiguities, so the user can see everything that
      // needs disambiguation in one error.
      if (ambiguous_seen.insert(out).second) {
        diag.ambiguous_outputs.push_back(out);
      }
      std::unordered_set<StateInterfaceId> visited;
      collect_blocking_ambiguities(reach, out, ambiguous_set, visited, blocking_ambiguities);
    } else {
      // Genuinely unreachable.
      if (unreachable_seen.insert(out).second) {
        diag.unreachable.push_back(out);
      }
    }
  }

  // Slice reach.ambiguities() to those that any blocked/ambiguous output depends on.
  for (const auto & amb : ambiguities) {
    if (blocking_ambiguities.count(amb.interface) > 0) {
      diag.relevant_blocking_ambiguities.push_back(amb);
    }
  }

  diag.resolutions = compute_missing_input_resolutions(reach, diag.unreachable);

  return diag;
}

std::vector<MissingInputResolution> compute_missing_input_resolutions(
  const TransmissionReachability & reach,
  const span<const StateInterfaceId> missing)
{
  (void)reach;
  // Stub: return one default-constructed entry per missing interface so callers can iterate
  // structurally even though the contents aren't useful yet.
  std::vector<MissingInputResolution> result;
  result.reserve(missing.size());
  for (const StateInterfaceId m : missing) {
    MissingInputResolution entry{};
    entry.missing = m;
    result.push_back(std::move(entry));
  }
  return result;
}

}  // namespace arm_kinematics
