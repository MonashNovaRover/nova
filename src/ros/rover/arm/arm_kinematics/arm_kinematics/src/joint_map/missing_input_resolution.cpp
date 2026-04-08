//
// Created by Bailey Chessum on 8/4/26.
//

#include "arm_kinematics/joint_map/missing_input_resolution.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "arm_kinematics/joint_map/transmission_reachability.hpp"

namespace arm_kinematics {

namespace {

// Walks `iface`'s producer chain and returns true if any node in the chain is an ambiguous
// interface (i.e. its producer is monostate AND it's listed in `reach.ambiguities()`).
//
// `ambiguous_set` is a precomputed lookup of every ambiguous interface id in the reachability,
// for O(1) checks. `cache` memoizes results across multiple calls (each interface is visited at
// most once across the whole diagnosis). `tainted_ambiguities` is populated with every
// ambiguous interface id encountered along *any* tainted chain — this is what feeds the
// builder's `relevant_ambiguities` slice.
bool depends_on_ambiguous(
  const TransmissionReachability & reach,
  const StateInterfaceId iface,
  const std::unordered_set<StateInterfaceId> & ambiguous_set,
  std::unordered_map<StateInterfaceId, bool> & cache,
  std::unordered_set<StateInterfaceId> & tainted_ambiguities)
{
  const auto cached = cache.find(iface);
  if (cached != cache.end()) {
    return cached->second;
  }
  // Tentatively mark as "not tainted" to break any pathological cycles. (The producer chain
  // is acyclic by construction — transmissions become viable strictly after their inputs —
  // but this is cheap insurance.)
  cache[iface] = false;

  bool tainted = false;
  const auto producer = reach.producer_of(iface);

  if (std::holds_alternative<std::monostate>(producer)) {
    // Either ambiguous or unreachable. If ambiguous, we taint THIS interface (it's a leaf
    // ambiguity).
    if (ambiguous_set.count(iface) > 0) {
      tainted_ambiguities.insert(iface);
      tainted = true;
    }
  } else if (std::holds_alternative<producers::Input>(producer)) {
    tainted = false;
  } else if (auto * tx = std::get_if<producers::Transmission>(&producer)) {
    const auto & instance = reach.analysis().transmissions()[tx->instance_id];
    for (const auto in : instance.input_ids) {
      if (depends_on_ambiguous(reach, in, ambiguous_set, cache, tainted_ambiguities)) {
        tainted = true;
        // Don't break — we want to discover *all* ambiguities the chain touches so the
        // builder can report them all in one error.
      }
    }
  } else if (auto * ap = std::get_if<producers::AffineProjection>(&producer)) {
    if (depends_on_ambiguous(reach, ap->source, ambiguous_set, cache, tainted_ambiguities)) {
      tainted = true;
    }
  }

  cache[iface] = tainted;
  return tainted;
}

}  // namespace

MissingOutputDiagnosis diagnose_missing_outputs(
  const TransmissionReachability & reach,
  const span<const StateInterfaceId> needed_outputs)
{
  MissingOutputDiagnosis diag{};

  // Build the ambiguity lookup set once.
  const auto ambiguities = reach.ambiguities();
  std::unordered_set<StateInterfaceId> ambiguous_set;
  ambiguous_set.reserve(ambiguities.size());
  for (const auto & amb : ambiguities) {
    ambiguous_set.insert(amb.interface);
  }

  std::unordered_map<StateInterfaceId, bool> taint_cache;
  std::unordered_set<StateInterfaceId> tainted_ambiguity_ids;

  std::unordered_set<StateInterfaceId> unreachable_seen;
  std::unordered_set<StateInterfaceId> ambiguous_seen;

  for (const StateInterfaceId out : needed_outputs) {
    const auto producer = reach.producer_of(out);

    if (std::holds_alternative<std::monostate>(producer)) {
      // Direct case: output is itself either ambiguous or unreachable.
      if (ambiguous_set.count(out) > 0) {
        if (ambiguous_seen.insert(out).second) {
          diag.ambiguous_outputs.push_back(out);
        }
        tainted_ambiguity_ids.insert(out);
      } else {
        if (unreachable_seen.insert(out).second) {
          diag.unreachable.push_back(out);
        }
      }
    } else {
      // Output has a producer. Walk the chain to detect transitive ambiguity poison.
      if (depends_on_ambiguous(reach, out, ambiguous_set, taint_cache, tainted_ambiguity_ids)) {
        if (ambiguous_seen.insert(out).second) {
          diag.ambiguous_outputs.push_back(out);
        }
      }
    }
  }

  // Slice reach.ambiguities() to those touched by tainted output chains.
  for (const auto & amb : ambiguities) {
    if (tainted_ambiguity_ids.count(amb.interface) > 0) {
      diag.relevant_ambiguities.push_back(amb);
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
