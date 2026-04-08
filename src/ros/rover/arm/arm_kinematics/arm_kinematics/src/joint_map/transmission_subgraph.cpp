//
// Created by nova on 5/4/26.
//

#include "arm_kinematics/joint_map/transmission_subgraph.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace arm_kinematics {

namespace {

// Reverse-lookup helper: given a StateInterfaceId, get its (joint_id, interface_id) parts via
// the analysis's state_interface_order_.inverse mapping. We need this for affine projection
// — the algorithm needs to know which joint and which interface id a given state interface
// belongs to.
const StateInterfaceDefinition & resolve_state_interface(
  const TransmissionAnalysis & analysis,
  const StateInterfaceId sid)
{
  return analysis.state_interface_order().inverse[sid];
}

// Helper for std::variant introspection.
bool is_input_producer(const StateInterfaceProducer & producer) noexcept
{
  return std::holds_alternative<producers::Input>(producer);
}

bool is_leaf_producer(const StateInterfaceProducer & producer) noexcept
{
  return std::holds_alternative<producers::Input>(producer) ||
         std::holds_alternative<producers::Transmission>(producer);
}

}  // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TransmissionSubgraph::TransmissionSubgraph(
  const TransmissionAnalysis & analysis,
  const span<const StateInterfaceId> initial_inputs,
  const span<const StateInterfaceId> initial_outputs)
  : analysis_(analysis),
    requested_inputs_(initial_inputs.begin(), initial_inputs.end()),
    needed_outputs_(initial_outputs.begin(), initial_outputs.end())
{
  run_fixed_point();
}

// ---------------------------------------------------------------------------
// Mutation
// ---------------------------------------------------------------------------

void TransmissionSubgraph::add_input(const StateInterfaceId interface)
{
  // First-occurrence-wins semantics: if `interface` is already in requested_inputs_, just
  // append the duplicate slot to the effective list and don't touch producer_assignment_.
  // Otherwise, this is a new addition that may override existing non-Input candidates.
  const bool was_already_present = std::find(
    requested_inputs_.begin(), requested_inputs_.end(), interface) != requested_inputs_.end();

  // Snapshot the prior candidate set BEFORE the change so we can log what gets discarded.
  // This covers three cases:
  //   - prior had a single non-Input producer → 1 discarded candidate
  //   - prior was ambiguous between multiple non-Input candidates → N discarded candidates
  //   - prior was monostate / unreachable → no discarded candidates, no override entry
  // (If the prior already had an Input producer, the addition is a duplicate and we wouldn't
  // be in this branch — was_already_present would be true.)
  std::vector<StateInterfaceProducer> prior_candidates;
  if (!was_already_present) {
    const auto pa_it = producer_assignment_.find(interface);
    if (pa_it != producer_assignment_.end() && !is_input_producer(pa_it->second)) {
      prior_candidates.push_back(pa_it->second);
    } else {
      for (const auto & ai : ambiguous_interfaces_) {
        if (ai.interface == interface) {
          prior_candidates = ai.candidates;
          break;
        }
      }
    }
  }

  // Always append to the effective input list.
  requested_inputs_.push_back(interface);

  // Re-run the fixed point. The new input becomes a producers::Input{...} for `interface`,
  // and Input wins over any non-Input candidates that may have existed.
  run_fixed_point();

  // If this was a fresh addition AND we discarded at least one non-Input candidate, log it.
  if (!was_already_present && !prior_candidates.empty()) {
    InputOverride entry{};
    entry.interface = interface;
    entry.discarded_candidates = std::move(prior_candidates);
    input_overrides_.push_back(std::move(entry));
  }
}

void TransmissionSubgraph::add_output(const StateInterfaceId interface)
{
  needed_outputs_.push_back(interface);
  run_fixed_point();
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool TransmissionSubgraph::is_complete() const noexcept
{
  return ambiguous_interfaces_.empty() && unreachable_outputs_.empty();
}

bool TransmissionSubgraph::is_ambiguous() const noexcept
{
  return !ambiguous_interfaces_.empty();
}

span<const StateInterfaceId> TransmissionSubgraph::requested_inputs() const noexcept
{
  return requested_inputs_;
}

span<const StateInterfaceId> TransmissionSubgraph::derivable_interfaces() const noexcept
{
  return derivable_interfaces_;
}

span<const StateInterfaceId> TransmissionSubgraph::unreachable_outputs() const noexcept
{
  return unreachable_outputs_;
}

span<const TransmissionSubgraph::AmbiguousInterface> TransmissionSubgraph::ambiguous_interfaces() const noexcept
{
  return ambiguous_interfaces_;
}

span<const TransmissionInstanceId> TransmissionSubgraph::selected_transmissions() const noexcept
{
  return selected_transmissions_;
}

bool TransmissionSubgraph::produces(const StateInterfaceId interface) const noexcept
{
  const auto it = producer_assignment_.find(interface);
  return it != producer_assignment_.end();
}

StateInterfaceProducer TransmissionSubgraph::producer_of(const StateInterfaceId interface) const noexcept
{
  const auto it = producer_assignment_.find(interface);
  if (it == producer_assignment_.end()) {
    return std::monostate{};
  }
  return it->second;
}

span<const TransmissionSubgraph::InputOverride> TransmissionSubgraph::input_overrides() const noexcept
{
  return input_overrides_;
}

span<const StateInterfaceId> TransmissionSubgraph::redundant_equivalent_inputs() const noexcept
{
  return redundant_equivalent_inputs_;
}

// ---------------------------------------------------------------------------
// Algorithm
// ---------------------------------------------------------------------------

TransmissionSubgraph::AffineProjectionCoefficients
TransmissionSubgraph::compute_affine_projection_coefficients(
  const JointId source_joint,
  const JointId target_joint,
  const AffineProjectionRule & rule) const noexcept
{
  // Both joints have a per-joint flat affine relation stored on the analysis:
  //   source = T_src.m * R + T_src.o
  //   target = T_tgt.m * R + T_tgt.o
  // where R is the affine group's root joint value (in joint-space). They share the same R
  // because both are in the same affine group.
  //
  // Solve T_src for R: R = (source - T_src.o) / T_src.m
  // Substitute into target: target = (T_tgt.m / T_src.m) * source + (T_tgt.o - T_tgt.m * T_src.o / T_src.m)
  //
  // So in joint-space, target = m_joint * source + o_joint where:
  //   m_joint = T_tgt.m / T_src.m
  //   o_joint = T_tgt.o - m_joint * T_src.o
  const auto & t_src = analysis_.affine_transmission_of(source_joint);
  const auto & t_tgt = analysis_.affine_transmission_of(target_joint);
  assert(t_src.multiplier != 0.0F && "compute_affine_projection_coefficients: source joint's flat multiplier is zero");

  const float joint_m = t_tgt.multiplier / t_src.multiplier;
  const float joint_o = t_tgt.offset - joint_m * t_src.offset;

  // Now apply the projection rule's interface-space transformation. The rule says:
  //   projected_target_iface = (multiplier_scale * raw_m) * projected_source_iface + (offset_scale * raw_o)
  // where raw_m and raw_o are the joint-space coefficients we just computed, and
  // reverse_direction swaps source/target before applying.
  //
  // For reverse_direction == false:
  //   target_iface = (multiplier_scale * joint_m) * source_iface + (offset_scale * joint_o)
  //
  // For reverse_direction == true:
  //   source_iface = (multiplier_scale * joint_m) * target_iface + (offset_scale * joint_o)
  // i.e., the relationship runs target → source. To get the source → target form (which is
  // what the planner records — "source's value gives target's value via this projection"),
  // we invert:
  //   target_iface = (1 / (multiplier_scale * joint_m)) * source_iface
  //                  - (offset_scale * joint_o) / (multiplier_scale * joint_m)
  AffineProjectionCoefficients result{};
  const float scaled_m = rule.multiplier_scale * joint_m;
  const float scaled_o = rule.offset_scale * joint_o;
  if (!rule.reverse_direction) {
    result.multiplier = scaled_m;
    result.offset = scaled_o;
  } else {
    // The convention here is that we always express the projection as
    //   target = m * source + o
    // so when the rule reverses direction we invert.
    assert(scaled_m != 0.0F && "compute_affine_projection_coefficients: reverse_direction with zero scaled multiplier");
    result.multiplier = 1.0F / scaled_m;
    result.offset = -scaled_o / scaled_m;
  }
  return result;
}

void TransmissionSubgraph::run_fixed_point()
{
  // ---------------------------------------------------------------------------
  // Reset derived state. We deliberately do NOT clear input_overrides_ — that log
  // is sticky across mutations (it accumulates events from add_input calls).
  // ---------------------------------------------------------------------------
  derivable_interfaces_.clear();
  derivable_membership_.assign(analysis_.state_interface_order().inverse.size(), false);
  producer_assignment_.clear();
  unreachable_outputs_.clear();
  ambiguous_interfaces_.clear();
  ambiguous_membership_.assign(analysis_.state_interface_order().inverse.size(), false);
  selected_transmissions_.clear();
  redundant_equivalent_inputs_.clear();

  const auto add_to_derivable = [this](const StateInterfaceId sid) {
    if (sid >= derivable_membership_.size()) {
      // Out of range — interface not in the analysis. Skip silently; the validation will
      // catch it elsewhere if it matters.
      return false;
    }
    if (derivable_membership_[sid]) {
      return false;
    }
    derivable_membership_[sid] = true;
    derivable_interfaces_.push_back(sid);
    return true;
  };

  // ---------------------------------------------------------------------------
  // Phase 1: Initialize from requested_inputs_, with first-occurrence-wins.
  // ---------------------------------------------------------------------------
  for (std::size_t i = 0; i < requested_inputs_.size(); ++i) {
    const StateInterfaceId sid = requested_inputs_[i];
    if (sid >= derivable_membership_.size()) {
      continue;  // out-of-range; skip silently
    }
    if (producer_assignment_.find(sid) == producer_assignment_.end()) {
      producer_assignment_[sid] = producers::Input{i};
      add_to_derivable(sid);
    }
    // else: duplicate, the first-occurrence Input producer wins
  }

  // ---------------------------------------------------------------------------
  // Phase 2: Forward fixed point. Build candidate lists for non-Input interfaces by walking
  // transmissions and affine projections until no new interfaces become derivable.
  // ---------------------------------------------------------------------------
  // Per-interface candidate accumulator. Interfaces that already have an Input producer in
  // producer_assignment_ are exempt — Input wins, no candidates recorded for them.
  std::unordered_map<StateInterfaceId, std::vector<StateInterfaceProducer>> candidates;

  const auto record_candidate = [&candidates, this](
    const StateInterfaceId iface,
    StateInterfaceProducer producer)
  {
    // Input-wins: if iface already has an Input producer, never record another candidate.
    const auto pa_it = producer_assignment_.find(iface);
    if (pa_it != producer_assignment_.end() && is_input_producer(pa_it->second)) {
      return;
    }
    candidates[iface].push_back(std::move(producer));
  };

  bool changed = true;
  while (changed) {
    changed = false;

    // ---- Transmissions ----
    // For each transmission whose inputs are all derivable, mark its outputs derivable and
    // record it as a candidate producer for each output.
    for (TransmissionInstanceId tid = 0; tid < analysis_.transmissions().size(); ++tid) {
      const auto & instance = analysis_.transmissions()[tid];
      bool all_inputs_known = true;
      for (const StateInterfaceId in : instance.input_ids) {
        if (in >= derivable_membership_.size() || !derivable_membership_[in]) {
          all_inputs_known = false;
          break;
        }
      }
      if (!all_inputs_known) {
        continue;
      }
      // T is viable. Record it as a candidate for each output (unless that output has Input).
      for (const StateInterfaceId out : instance.output_ids) {
        // Avoid duplicate candidates if the same transmission is reached again in a later
        // iteration (it shouldn't be, but defend against bugs).
        bool already_listed = false;
        const auto cand_it = candidates.find(out);
        if (cand_it != candidates.end()) {
          for (const auto & c : cand_it->second) {
            if (auto * tx = std::get_if<producers::Transmission>(&c)) {
              if (tx->instance_id == tid) {
                already_listed = true;
                break;
              }
            }
          }
        }
        if (!already_listed) {
          record_candidate(out, producers::Transmission{tid});
        }
        if (add_to_derivable(out)) {
          changed = true;
        }
      }
    }

    // ---- Affine projections ----
    // Group derivable interfaces by (affine_root, interface_id). For each such group, find the
    // lowest-JointId leaf that's currently derivable and use it as the source for AffineProjection
    // candidates targeting other (member, interface_id) pairs.
    //
    // Snapshot the derivable list to avoid iterator invalidation if we add to it during the pass.
    const auto derivable_snapshot = derivable_interfaces_;
    // Dedup (group root, interface id hash) processing within this iteration. We use a
    // std::set<std::pair> so two distinct (root, interface) pairs cannot collide on a single
    // size_t hash and accidentally skip one.
    std::set<std::pair<JointId, std::size_t>> processed_groups;
    for (const StateInterfaceId sid : derivable_snapshot) {
      const auto & defn = resolve_state_interface(analysis_, sid);
      const AffineProjectionRule * rule = analysis_.affine_projection_rule(defn.interface_id);
      if (rule == nullptr) {
        continue;  // No projection rule for this interface id — affine doesn't propagate.
      }
      const JointId root = analysis_.affine_root_of(defn.joint_id);
      const auto group_members = analysis_.affine_group_members(root);
      if (group_members.size() <= 1) {
        continue;  // Trivial group, nothing to project to.
      }
      if (!processed_groups.insert({root, defn.interface_id.hash}).second) {
        continue;
      }

      // Find the lowest-JointId leaf in the group whose (joint, interface_id) is currently
      // derivable AND whose producer is a leaf (Input or Transmission).
      //
      // A member counts as a leaf source if either:
      //   (a) it has an Input or Transmission entry already committed in producer_assignment_
      //       (Input is committed in phase 1; Transmissions only become committed in phase 3,
      //       so during the fixed-point loop only Inputs land here), OR
      //   (b) it has exactly one Transmission candidate in `candidates` and is not yet committed
      //       — this lets affine projections chain off transmission outputs during the loop. If
      //       a second transmission later joins the candidate set, the member becomes ambiguous,
      //       producer_of() returns monostate, and the AffineProjection.source pointer dangles
      //       from the consumer's perspective — this is the same ambiguity-poison case the class
      //       doc warns about, masked by is_complete().
      JointId source_joint = std::numeric_limits<JointId>::max();
      bool found_leaf_source = false;
      // Track all leaf-Input candidates so we can populate redundant_equivalent_inputs_.
      std::vector<JointId> leaf_input_joints_in_group;
      for (const JointId member : group_members) {
        // Look up (member, interface_id) — must already be in state_interface_order_ for the
        // analysis to know about it. If not, skip.
        const auto member_def = StateInterfaceDefinition{member, defn.interface_id};
        if (!analysis_.state_interface_order().contains_key(member_def)) {
          continue;
        }
        const StateInterfaceId member_sid = analysis_.state_interface_order()[member_def];
        if (member_sid >= derivable_membership_.size() || !derivable_membership_[member_sid]) {
          continue;
        }
        bool is_leaf_source = false;
        bool is_input_leaf = false;
        const auto pa_it = producer_assignment_.find(member_sid);
        if (pa_it != producer_assignment_.end() && is_leaf_producer(pa_it->second)) {
          is_leaf_source = true;
          is_input_leaf = is_input_producer(pa_it->second);
        } else if (pa_it == producer_assignment_.end()) {
          // Not yet committed — check candidates for a unique Transmission candidate.
          const auto cand_it = candidates.find(member_sid);
          if (cand_it != candidates.end() && cand_it->second.size() == 1 &&
              std::holds_alternative<producers::Transmission>(cand_it->second.front()))
          {
            is_leaf_source = true;
          }
        }
        if (!is_leaf_source) {
          continue;
        }
        if (member < source_joint) {
          source_joint = member;
          found_leaf_source = true;
        }
        if (is_input_leaf) {
          leaf_input_joints_in_group.push_back(member);
        }
      }
      if (!found_leaf_source) {
        continue;  // No leaf source available; can't form an AffineProjection.
      }

      // Record redundant_equivalent_inputs: every leaf-Input joint in the group OTHER than
      // source_joint counts as a redundant equivalent input. Avoid duplicates if the user
      // mutates the subgraph multiple times — we cleared the list at the start of the pass.
      for (const JointId leaf_joint : leaf_input_joints_in_group) {
        if (leaf_joint == source_joint) continue;
        // The redundant *interface* is (leaf_joint, defn.interface_id).
        const auto leaf_def = StateInterfaceDefinition{leaf_joint, defn.interface_id};
        const StateInterfaceId leaf_sid = analysis_.state_interface_order()[leaf_def];
        if (std::find(redundant_equivalent_inputs_.begin(),
                      redundant_equivalent_inputs_.end(),
                      leaf_sid) == redundant_equivalent_inputs_.end())
        {
          redundant_equivalent_inputs_.push_back(leaf_sid);
        }
      }

      // Get the source's StateInterfaceId (will be the source field of the AffineProjection).
      const auto source_def = StateInterfaceDefinition{source_joint, defn.interface_id};
      const StateInterfaceId source_sid = analysis_.state_interface_order()[source_def];

      // For each non-source member of the group, create an AffineProjection candidate
      // (if the (member, interface) is in the analysis and not already an Input).
      for (const JointId member : group_members) {
        if (member == source_joint) continue;
        const auto member_def = StateInterfaceDefinition{member, defn.interface_id};
        if (!analysis_.state_interface_order().contains_key(member_def)) {
          continue;  // analysis doesn't know about (member, this interface_id)
        }
        const StateInterfaceId member_sid = analysis_.state_interface_order()[member_def];
        // Compute the (m, o) from source_sid to member_sid in interface space.
        const auto coeffs = compute_affine_projection_coefficients(source_joint, member, *rule);
        // Record the candidate (Input-wins exempts the target if applicable).
        producers::AffineProjection projection{};
        projection.source = source_sid;
        projection.multiplier = coeffs.multiplier;
        projection.offset = coeffs.offset;
        // Avoid duplicate candidates if we revisit this group on a later iteration.
        bool already_listed = false;
        const auto cand_it = candidates.find(member_sid);
        if (cand_it != candidates.end()) {
          for (const auto & c : cand_it->second) {
            if (auto * ap = std::get_if<producers::AffineProjection>(&c)) {
              if (ap->source == projection.source &&
                  ap->multiplier == projection.multiplier &&
                  ap->offset == projection.offset)
              {
                already_listed = true;
                break;
              }
            }
          }
        }
        if (!already_listed) {
          record_candidate(member_sid, projection);
        }
        if (add_to_derivable(member_sid)) {
          changed = true;
        }
      }
    }
  }

  // ---------------------------------------------------------------------------
  // Phase 3: Classify the candidate set. 1 candidate → commit; ≥2 → ambiguous.
  // ---------------------------------------------------------------------------
  for (auto & kv : candidates) {
    const StateInterfaceId iface = kv.first;
    auto & cand_list = kv.second;
    if (cand_list.empty()) {
      continue;
    }
    if (cand_list.size() == 1) {
      // Don't overwrite an Input producer (input-wins).
      if (producer_assignment_.find(iface) == producer_assignment_.end()) {
        producer_assignment_[iface] = std::move(cand_list.front());
      }
    } else {
      AmbiguousInterface entry{};
      entry.interface = iface;
      entry.candidates = std::move(cand_list);
      ambiguous_interfaces_.push_back(std::move(entry));
      if (iface < ambiguous_membership_.size()) {
        ambiguous_membership_[iface] = true;
      }
      // Ambiguous interfaces have NO entry in producer_assignment_ — producer_of() returns
      // monostate for them.
    }
  }

  // ---------------------------------------------------------------------------
  // Phase 4: Classify needed outputs. Anything not in producer_assignment_ and not in the
  // ambiguous set is unreachable.
  // ---------------------------------------------------------------------------
  for (const StateInterfaceId out : needed_outputs_) {
    if (producer_assignment_.find(out) != producer_assignment_.end()) {
      continue;  // satisfied
    }
    if (out < ambiguous_membership_.size() && ambiguous_membership_[out]) {
      continue;  // ambiguous, already counted
    }
    if (std::find(unreachable_outputs_.begin(), unreachable_outputs_.end(), out) ==
        unreachable_outputs_.end())
    {
      unreachable_outputs_.push_back(out);
    }
  }

  // ---------------------------------------------------------------------------
  // Phase 5: Topological emission of selected_transmissions_.
  // ---------------------------------------------------------------------------
  emit_selected_transmissions();
}

void TransmissionSubgraph::emit_selected_transmissions()
{
  // Walk back from every needed output that has a unique Transmission/AffineProjection producer
  // and collect every TransmissionInstanceId that's transitively required. Then topologically
  // sort.
  std::unordered_set<TransmissionInstanceId> selected_set;

  // Worklist of state interfaces to trace.
  std::vector<StateInterfaceId> to_trace;
  for (const StateInterfaceId out : needed_outputs_) {
    to_trace.push_back(out);
  }
  std::unordered_set<StateInterfaceId> traced;

  while (!to_trace.empty()) {
    const StateInterfaceId iface = to_trace.back();
    to_trace.pop_back();
    if (!traced.insert(iface).second) {
      continue;
    }
    const auto pa_it = producer_assignment_.find(iface);
    if (pa_it == producer_assignment_.end()) {
      continue;  // unreachable / ambiguous / monostate
    }
    const auto & producer = pa_it->second;
    if (auto * tx = std::get_if<producers::Transmission>(&producer)) {
      if (selected_set.insert(tx->instance_id).second) {
        // Newly selected; trace its inputs too.
        const auto & instance = analysis_.transmissions()[tx->instance_id];
        for (const StateInterfaceId in : instance.input_ids) {
          to_trace.push_back(in);
        }
      }
    } else if (auto * ap = std::get_if<producers::AffineProjection>(&producer)) {
      // Affine projection — trace its source (which is a leaf by invariant).
      to_trace.push_back(ap->source);
    }
    // Input → no further tracing
  }

  // Topological sort: a transmission T1 must come before T2 if any of T2's inputs are
  // produced by T1. We have a small selected set, so a Kahn's-algorithm pass is fine.
  std::vector<TransmissionInstanceId> selected_list(selected_set.begin(), selected_set.end());
  // Sort by id first for determinism, then topologically.
  std::sort(selected_list.begin(), selected_list.end());

  // Build dependency edges: edge T1 → T2 iff T2 has an input that's produced by T1 (i.e.
  // producer_of(input) is producers::Transmission{T1}).
  std::unordered_map<TransmissionInstanceId, std::vector<TransmissionInstanceId>> dependents;
  std::unordered_map<TransmissionInstanceId, std::size_t> in_degree;
  for (const auto tid : selected_list) {
    in_degree[tid] = 0;
  }
  for (const auto t2 : selected_list) {
    const auto & instance = analysis_.transmissions()[t2];
    for (const StateInterfaceId in : instance.input_ids) {
      const auto pa_it = producer_assignment_.find(in);
      if (pa_it == producer_assignment_.end()) continue;
      // Walk through AffineProjection chains to find the underlying transmission, if any.
      const StateInterfaceProducer * cur = &pa_it->second;
      while (cur) {
        if (auto * tx = std::get_if<producers::Transmission>(cur)) {
          const TransmissionInstanceId t1 = tx->instance_id;
          if (selected_set.count(t1) > 0 && t1 != t2) {
            dependents[t1].push_back(t2);
            in_degree[t2]++;
          }
          break;
        } else if (auto * ap = std::get_if<producers::AffineProjection>(cur)) {
          const auto src_it = producer_assignment_.find(ap->source);
          if (src_it == producer_assignment_.end()) break;
          cur = &src_it->second;
        } else {
          break;
        }
      }
    }
  }

  // Kahn's: process zero-in-degree nodes in JointId order for determinism.
  std::vector<TransmissionInstanceId> ready;
  for (const auto tid : selected_list) {
    if (in_degree[tid] == 0) {
      ready.push_back(tid);
    }
  }
  std::sort(ready.begin(), ready.end());

  while (!ready.empty()) {
    const TransmissionInstanceId tid = ready.front();
    ready.erase(ready.begin());
    selected_transmissions_.push_back(tid);
    auto deps_it = dependents.find(tid);
    if (deps_it == dependents.end()) continue;
    for (const TransmissionInstanceId dep : deps_it->second) {
      if (--in_degree[dep] == 0) {
        // Insert in sorted order to keep output deterministic.
        const auto pos = std::lower_bound(ready.begin(), ready.end(), dep);
        ready.insert(pos, dep);
      }
    }
  }
  // By construction the dependency graph is a DAG (each transmission becomes viable strictly
  // after its inputs are derivable, so a cycle would require a transmission's input to be
  // produced by a transmission that depends on it — impossible). Catch any future bugs that
  // might break this property loudly in debug builds.
  assert(selected_transmissions_.size() == selected_set.size() &&
         "topological sort produced a partial order — cycle in selected transmissions?");
}

}  // namespace arm_kinematics
