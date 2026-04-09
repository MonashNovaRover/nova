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
// every ambiguous-interface ancestor (transitively, through transmission inputs and affine
// projection sources). Used only on the failing path to attribute "this output is blocked
// because of these upstream ambiguities".
//
// `ambiguous_set` is the precomputed lookup of every ambiguous interface in the reachability,
// for O(1) checks.
//
// The walk is bounded by the number of state interfaces (visited set prevents revisits).
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
    return;  // Don't walk past an ambiguous interface.
  }
  // Walk every potential producer transmission for this interface and recurse on its inputs.
  const auto producing = reach.analysis().producing_transmissions(blocked_output);
  for (const auto tid : producing) {
    const auto & instance = reach.analysis().transmissions()[tid];
    for (const auto in : instance.input_ids) {
      collect_blocking_ambiguities(reach, in, ambiguous_set, visited, blocking_ambiguities);
    }
  }
  // Note: affine projection blocking attribution is conservative — we don't walk through
  // affine groups here, because the producing_transmissions index doesn't cover affine
  // sources. The reachability has already classified the interface as blocked, so the user
  // gets the correct error category (Ambiguous); we just may miss some root-cause attribution
  // in pure-affine cases. Refinement deferred.
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
    if (ambiguous_set.count(out) > 0) {
      // Directly ambiguous: the output itself has multiple producers.
      if (ambiguous_seen.insert(out).second) {
        diag.ambiguous_outputs.push_back(out);
      }
      blocking_ambiguities.insert(out);
    } else if (blocked_set.count(out) > 0) {
      // Blocked: producer chain transitively depends on an ambiguous upstream.
      if (ambiguous_seen.insert(out).second) {
        diag.ambiguous_outputs.push_back(out);
      }
      // Walk back to find which upstream ambiguities are responsible.
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
