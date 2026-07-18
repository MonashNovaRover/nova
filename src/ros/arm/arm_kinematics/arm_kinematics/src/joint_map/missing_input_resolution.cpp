//
// Created by Bailey Chessum on 8/4/26.
//

#include "arm_kinematics/joint_map/missing_input_resolution.hpp"

#include <algorithm>
#include <unordered_set>

#include "arm_kinematics/joint_map/transmission_reachability.hpp"

namespace arm_kinematics {

using StateInterfaceId = TransmissionReachability::StateInterfaceId;

namespace {

// Walk the analysis's potential-producer graph backward from `blocked_output` (SID) and
// collect every ambiguous-interface ancestor. Used only on the failing path to attribute
// "this output is blocked because of these upstream ambiguities".
//
// `ambiguous_set` is the precomputed lookup of every ambiguous interface in the reachability.
// The walk is bounded by the visited set.
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
  }

  // ---- Walk producing transmissions ----
  const auto & analysis = reach.analysis();
  const auto producing = analysis.producing_transmissions(blocked_output);
  for (const auto tid : producing) {
    const auto & instance = analysis.transmissions()[tid];
    for (const auto in : instance.input_ids) {
      collect_blocking_ambiguities(reach, in, ambiguous_set, visited, blocking_ambiguities);
    }
  }

  // ---- Walk affine group members ----
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
  const span<const StateInterfaceDefinition> needed_outputs)
{
  MissingOutputDiagnosis diag{};

  const auto ambiguities = reach.ambiguities();
  const auto & analysis = reach.analysis();

  // Build SID-indexed ambiguity set for O(1) lookup inside collect_blocking_ambiguities.
  std::unordered_set<StateInterfaceId> ambiguous_set;
  ambiguous_set.reserve(ambiguities.size());
  for (const auto & amb : ambiguities) {
    if (analysis.state_interface_order().contains_key(amb.interface)) {
      ambiguous_set.insert(analysis.state_interface_order()[amb.interface]);
    }
  }

  const auto transitively_blocked_span = reach.transitively_blocked_interfaces();
  std::unordered_set<StateInterfaceId> transitively_blocked_set;
  transitively_blocked_set.reserve(transitively_blocked_span.size());
  for (const auto sid : transitively_blocked_span) {
    transitively_blocked_set.insert(sid);
  }

  std::unordered_set<StateInterfaceDefinition> unproducible_seen;
  std::unordered_set<StateInterfaceDefinition> directly_ambiguous_seen;
  std::unordered_set<StateInterfaceDefinition> transitively_blocked_seen;
  std::unordered_set<StateInterfaceId> blocking_ambiguities;

  for (const StateInterfaceDefinition & out_def : needed_outputs) {
    const auto producer = reach.producer_of_def(out_def);
    if (!std::holds_alternative<std::monostate>(producer)) {
      continue;  // satisfied
    }

    // Determine SID (if any) for ambiguity/transitively-blocked classification.
    const auto opt_sid = analysis.find_state_interface_id(out_def);

    const bool is_ambiguous = opt_sid.has_value() && ambiguous_set.count(*opt_sid) > 0;
    const bool is_transitively_blocked =
      opt_sid.has_value() && transitively_blocked_set.count(*opt_sid) > 0;

    if (is_ambiguous) {
      if (directly_ambiguous_seen.insert(out_def).second) {
        diag.directly_ambiguous_outputs.push_back(out_def);
      }
      std::unordered_set<StateInterfaceId> visited;
      collect_blocking_ambiguities(reach, *opt_sid, ambiguous_set, visited, blocking_ambiguities);
    } else if (is_transitively_blocked) {
      if (transitively_blocked_seen.insert(out_def).second) {
        diag.transitively_blocked_outputs.push_back(out_def);
      }
      std::unordered_set<StateInterfaceId> visited;
      collect_blocking_ambiguities(reach, *opt_sid, ambiguous_set, visited, blocking_ambiguities);
    } else {
      // Genuinely unproducible (no SID, or SID not in any ambiguous/blocked set).
      if (unproducible_seen.insert(out_def).second) {
        diag.unproducible.push_back(out_def);
      }
    }
  }

  // Slice reach.ambiguities() to those that any failing output depends on.
  for (const auto & amb : ambiguities) {
    if (analysis.state_interface_order().contains_key(amb.interface)) {
      const StateInterfaceId sid = analysis.state_interface_order()[amb.interface];
      if (blocking_ambiguities.count(sid) > 0) {
        diag.relevant_blocking_ambiguities.push_back(amb);
      }
    }
  }

  diag.resolutions = compute_missing_input_resolutions(reach, diag.unproducible);

  return diag;
}

std::vector<MissingInputResolution> compute_missing_input_resolutions(
  const TransmissionReachability & reach,
  const span<const StateInterfaceDefinition> missing)
{
  std::vector<MissingInputResolution> result;
  result.reserve(missing.size());

  // Build the "currently supplied" set from the reachability's inputs (defs, deduped).
  std::unordered_set<StateInterfaceDefinition> supplied_set;
  supplied_set.reserve(reach.inputs().size());
  for (const auto & def : reach.inputs()) {
    supplied_set.insert(def);
  }

  const auto & analysis = reach.analysis();

  for (const StateInterfaceDefinition & m_def : missing) {
    MissingInputResolution entry{};
    entry.missing = m_def;

    // ---- Transmission alternatives ----
    // Only possible for defs with a registered SID (bare defs have no transmission producers).
    const auto opt_sid = analysis.find_state_interface_id(m_def);
    if (opt_sid.has_value()) {
      const auto producing = analysis.producing_transmissions(*opt_sid);
      for (const auto tid : producing) {
        const auto & instance = analysis.transmissions()[tid];
        std::vector<StateInterfaceDefinition> needed;
        for (const auto in_sid : instance.input_ids) {
          const StateInterfaceDefinition in_def = analysis.state_interface_order().inverse[in_sid];
          if (supplied_set.count(in_def) == 0) {
            needed.push_back(in_def);
          }
        }
        if (needed.empty()) {
          continue;  // all inputs already supplied — shouldn't be missing, skip defensively
        }
        entry.transmission_alternatives.push_back(std::move(needed));
      }
    }

    // ---- Affine alternative ----
    const AffineProjectionRule * rule = analysis.affine_projection_rule(m_def.interface_id);
    if (rule != nullptr) {
      const JointId root = analysis.affine_root_of(m_def.joint_id);
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
