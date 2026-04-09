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
  std::unordered_set<StateInterfaceId> directly_ambiguous_seen;
  std::unordered_set<StateInterfaceId> blocked_seen;
  std::unordered_set<StateInterfaceId> blocking_ambiguities;

  for (const StateInterfaceId out : needed_outputs) {
    const auto producer = reach.producer_of(out);
    if (!std::holds_alternative<std::monostate>(producer)) {
      continue;  // satisfied
    }
    if (ambiguous_set.count(out) > 0) {
      // Directly ambiguous: the output itself has ≥2 viable producers in the analysis.
      if (directly_ambiguous_seen.insert(out).second) {
        diag.directly_ambiguous_outputs.push_back(out);
      }
      // Walk the producer chain to attribute deeper upstream ambiguities (in case the user
      // also needs to fix something further upstream).
      std::unordered_set<StateInterfaceId> visited;
      collect_blocking_ambiguities(reach, out, ambiguous_set, visited, blocking_ambiguities);
    } else if (blocked_set.count(out) > 0) {
      // Blocked: producer chain transitively depends on an ambiguous upstream.
      if (blocked_seen.insert(out).second) {
        diag.blocked_outputs.push_back(out);
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
  // For each missing interface, enumerate two kinds of remediation:
  //
  //   1. **Transmission alternatives**: any transmission T whose outputs include the missing
  //      interface and whose inputs are NOT all already supplied. The "needed" set is the
  //      subset of T's inputs that are not currently in the reachability's input set. If a
  //      transmission's needed set is empty, T should already be firing — that means the
  //      missing interface should already be derivable, so we skip it (this case shouldn't
  //      happen in a correct caller, but we handle it defensively).
  //
  //   2. **Affine alternative**: if the missing interface lives in a non-trivial affine
  //      group AND its interface_id has a registered projection rule, populate `affine_root`
  //      with the group's root joint id. The user can supply ANY other group member at the
  //      same interface_id to unblock the missing one.
  //
  // The algorithm doesn't recursively check whether the recommended inputs are themselves
  // unreachable — the user iterates: supply X, retry, see what's now missing, repeat.
  //
  // Complexity: O(K × (P × m + s)) where K = len(missing), P = max producers per interface,
  // m = max inputs per transmission, s = max affine group size. Only runs on the failing
  // path.

  std::vector<MissingInputResolution> result;
  result.reserve(missing.size());

  // Build the "currently supplied" set from the reachability's inputs (deduped).
  std::unordered_set<StateInterfaceId> supplied_set;
  supplied_set.reserve(reach.inputs().size());
  for (const auto sid : reach.inputs()) {
    supplied_set.insert(sid);
  }

  const auto & analysis = reach.analysis();

  for (const StateInterfaceId m : missing) {
    MissingInputResolution entry{};
    entry.missing = m;

    // ---- Transmission alternatives ----
    const auto producing = analysis.producing_transmissions(m);
    for (const auto tid : producing) {
      const auto & instance = analysis.transmissions()[tid];
      std::vector<StateInterfaceId> needed;
      for (const auto in : instance.input_ids) {
        if (supplied_set.count(in) == 0) {
          needed.push_back(in);
        }
      }
      // If needed is empty, the transmission already had all inputs available — meaning the
      // missing interface should NOT be missing. Skip this entry (defensive; shouldn't happen
      // for a correct caller).
      if (needed.empty()) {
        continue;
      }
      entry.transmission_alternatives.push_back(std::move(needed));
    }

    // ---- Affine alternative ----
    const auto & defn = analysis.state_interface_order().inverse[m];
    const AffineProjectionRule * rule = analysis.affine_projection_rule(defn.interface_id);
    if (rule != nullptr) {
      const JointId root = analysis.affine_root_of(defn.joint_id);
      const auto group_members = analysis.affine_group_members(root);
      if (group_members.size() > 1) {
        entry.affine_root = root;
      }
    }

    result.push_back(std::move(entry));
  }

  return result;
}

}  // namespace arm_kinematics
