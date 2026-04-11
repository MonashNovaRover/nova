//
// Created by Bailey Chessum on 8/4/26.
//

#include "arm_kinematics/joint_map/transmission_reachability.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <set>
#include <unordered_map>
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

TransmissionReachability TransmissionReachability::analyze(
  const TransmissionAnalysis & analysis,
  const span<const StateInterfaceId> inputs)
{
  // Direct prvalue return — guaranteed copy elision (C++17) materializes the constructed
  // object in the caller's storage. The type is non-movable so this is the only path.
  return TransmissionReachability(analysis, inputs);
}

TransmissionReachability::TransmissionReachability(
  const TransmissionAnalysis & analysis,
  const span<const StateInterfaceId> inputs)
  : analysis_(analysis),
    inputs_(inputs.begin(), inputs.end())
{
  run_fixed_point(inputs);
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool TransmissionReachability::is_ambiguous() const noexcept
{
  return !ambiguities_.empty();
}

span<const StateInterfaceId> TransmissionReachability::inputs() const noexcept
{
  return inputs_;
}

span<const StateInterfaceId> TransmissionReachability::derivable_interfaces() const noexcept
{
  return derivable_interfaces_;
}

span<const TransmissionReachability::AmbiguousInterface>
TransmissionReachability::ambiguities() const noexcept
{
  return ambiguities_;
}

span<const StateInterfaceId> TransmissionReachability::transitively_blocked_interfaces() const noexcept
{
  return transitively_blocked_interfaces_;
}

span<const StateInterfaceId> TransmissionReachability::unproducible_interfaces() const noexcept
{
  return unproducible_interfaces_;
}

StateInterfaceProducer TransmissionReachability::producer_of(
  const StateInterfaceId interface) const noexcept
{
  const auto it = producer_assignment_.find(interface);
  if (it == producer_assignment_.end()) {
    return std::monostate{};
  }
  return it->second;
}

span<const StateInterfaceId> TransmissionReachability::redundant_equivalent_inputs() const noexcept
{
  return redundant_equivalent_inputs_;
}

// ---------------------------------------------------------------------------
// Algorithm
// ---------------------------------------------------------------------------

TransmissionReachability::AffineProjectionCoefficients
TransmissionReachability::compute_affine_projection_coefficients(
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

  const double joint_m = t_tgt.multiplier / t_src.multiplier;
  const double joint_o = t_tgt.offset - joint_m * t_src.offset;

  // Now apply the projection rule's interface-space transformation. See
  // affine_projection_rule.hpp for the formal definition; in short:
  //   reverse_direction == false → target_iface = (mscale*joint_m) * source_iface + (oscale*joint_o)
  //   reverse_direction == true  → invert so the projection still reads source → target.
  AffineProjectionCoefficients result{};
  const double scaled_m = rule.multiplier_scale * joint_m;
  const double scaled_o = rule.offset_scale * joint_o;
  if (!rule.reverse_direction) {
    result.multiplier = scaled_m;
    result.offset = scaled_o;
  } else {
    assert(scaled_m != 0.0F && "compute_affine_projection_coefficients: reverse_direction with zero scaled multiplier");
    result.multiplier = 1.0F / scaled_m;
    result.offset = -scaled_o / scaled_m;
  }
  return result;
}

bool TransmissionReachability::add_to_derivable(const StateInterfaceId sid)
{
  if (sid >= derivable_membership_.size()) {
    return false;
  }
  if (derivable_membership_[sid]) {
    return false;
  }
  derivable_membership_[sid] = true;
  derivable_interfaces_.push_back(sid);
  return true;
}

void TransmissionReachability::record_candidate(
  const StateInterfaceId iface,
  StateInterfaceProducer producer,
  std::unordered_map<StateInterfaceId, std::vector<StateInterfaceProducer>> & candidates) const
{
  // Input-wins: if iface already has an Input producer, never record another candidate.
  const auto pa_it = producer_assignment_.find(iface);
  if (pa_it != producer_assignment_.end() && is_input_producer(pa_it->second)) {
    return;
  }
  candidates[iface].push_back(std::move(producer));
}

void TransmissionReachability::process_affine_hypernode(
  const InterfaceId & interface_id,
  const AffineProjectionRule & rule,
  const span<const JointId> group_members,
  std::unordered_map<StateInterfaceId, std::vector<StateInterfaceProducer>> & candidates,
  bool & changed)
{
  // The hypernode's "root" from the union-find is intentionally NOT used. The actual source
  // picking is done dynamically below — whichever derivable group member happens to be a
  // leaf wins. This is what enables non-root members to be supplied as inputs.

  const std::size_t state_count = analysis_.state_interface_order().inverse.size();

  // ---- Find the lowest-JointId leaf source for this hypernode -------------
  // A member is a "leaf source" candidate if either:
  //   (a) it has an Input or Transmission entry already committed in producer_assignment_
  //       (Input is committed in phase 1; Transmissions only become committed at the very end,
  //       so during the inner loop only Inputs land here), OR
  //   (b) it has exactly one Transmission candidate in `candidates` and is not yet committed
  //       — this lets affine projections chain off transmission outputs in the same pass.
  // Ambiguous (already-known-ambiguous) members are skipped — they cannot serve as sources.
  JointId source_joint = std::numeric_limits<JointId>::max();
  bool found_leaf_source = false;
  // Track all leaf-Input members so we can populate redundant_equivalent_inputs_ for any
  // duplicates beyond the lowest-JointId one.
  std::vector<JointId> leaf_input_joints_in_group;
  for (const JointId member : group_members) {
    const auto member_def = StateInterfaceDefinition{member, interface_id};
    if (!analysis_.state_interface_order().contains_key(member_def)) continue;
    const StateInterfaceId member_sid = analysis_.state_interface_order()[member_def];
    if (member_sid >= state_count || !derivable_membership_[member_sid]) continue;
    if (ambiguous_membership_[member_sid]) continue;  // ambiguity-honest

    bool is_leaf_source = false;
    bool is_input_leaf = false;
    const auto pa_it = producer_assignment_.find(member_sid);
    if (pa_it != producer_assignment_.end() && is_leaf_producer(pa_it->second)) {
      is_leaf_source = true;
      is_input_leaf = is_input_producer(pa_it->second);
    } else if (pa_it == producer_assignment_.end()) {
      const auto cand_it = candidates.find(member_sid);
      if (cand_it != candidates.end() && cand_it->second.size() == 1 &&
          std::holds_alternative<producers::Transmission>(cand_it->second.front()))
      {
        is_leaf_source = true;
      }
    }
    if (!is_leaf_source) continue;
    if (member < source_joint) {
      source_joint = member;
      found_leaf_source = true;
    }
    if (is_input_leaf) {
      leaf_input_joints_in_group.push_back(member);
    }
  }
  if (!found_leaf_source) return;

  // ---- Record redundant_equivalent_inputs ----------------------------------
  // Every leaf-Input joint in the group OTHER than the chosen source counts as a redundant
  // equivalent input — the user supplied two mathematically-equivalent values; the algorithm
  // honors them both as Input but uses only one as the projection source.
  for (const JointId leaf_joint : leaf_input_joints_in_group) {
    if (leaf_joint == source_joint) continue;
    const auto leaf_def = StateInterfaceDefinition{leaf_joint, interface_id};
    const StateInterfaceId leaf_sid = analysis_.state_interface_order()[leaf_def];
    if (std::find(redundant_equivalent_inputs_.begin(),
                  redundant_equivalent_inputs_.end(),
                  leaf_sid) == redundant_equivalent_inputs_.end())
    {
      redundant_equivalent_inputs_.push_back(leaf_sid);
    }
  }

  // ---- Project from source to every other group member --------------------
  const auto source_def = StateInterfaceDefinition{source_joint, interface_id};
  const StateInterfaceId source_sid = analysis_.state_interface_order()[source_def];

  for (const JointId member : group_members) {
    if (member == source_joint) continue;
    const auto member_def = StateInterfaceDefinition{member, interface_id};
    if (!analysis_.state_interface_order().contains_key(member_def)) continue;
    const StateInterfaceId member_sid = analysis_.state_interface_order()[member_def];
    const auto coeffs = compute_affine_projection_coefficients(source_joint, member, rule);
    producers::AffineProjection projection{};
    projection.source = source_sid;
    projection.multiplier = coeffs.multiplier;
    projection.offset = coeffs.offset;
    // Dedup against existing identical AffineProjection candidates for this member.
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
      record_candidate(member_sid, projection, candidates);
    }
    if (!ambiguous_membership_[member_sid]) {
      if (add_to_derivable(member_sid)) {
        changed = true;
      }
    }
  }
}

void TransmissionReachability::run_fixed_point(const span<const StateInterfaceId> inputs)
{
  // ===========================================================================
  // Algorithm overview & complexity
  // ===========================================================================
  //
  // This is a 2-pass forward dataflow that classifies every state interface in the analysis
  // into exactly one of {derivable, ambiguous, transitively_blocked, unproducible}.
  //
  // Variables (used in the complexity bounds below):
  //   N = number of state interfaces in the analysis
  //   T = number of transmissions in the analysis
  //   G = number of distinct affine groups with a registered projection rule
  //   m = max inputs/outputs per transmission (typically 1–4)
  //   s = max members per affine group (typically 4–6)
  //   d = depth of the longest derivability chain (longest sequence T_1, T_2, ..., T_k where
  //       each T_i reads an output of T_{i-1}); typically 3–10
  //   K_amb = number of ambiguous interfaces in the analysis (usually 0)
  //
  // Structure:
  //
  //   Pass 1 (always): unrestricted forward fixed point. ambiguous_membership_ is empty, so
  //     every transmission with derivable inputs fires. After convergence, classify candidates:
  //     any interface with ≥2 candidates is ambiguous, and we snapshot its FULL candidate list
  //     (this is the maximum it will ever have, by the monotonic-shrinking argument below).
  //
  //   Pass 2 (only if pass 1 found any ambiguities): restricted forward fixed point. The
  //     transmission viability check now skips any transmission whose inputs include an
  //     interface in `ambiguous_membership_`. The result is the final, ambiguity-honest
  //     derivable set.
  //
  //   Final commit: walk the pass-2 candidates map (or pass-1 if pass 2 didn't run). Commit
  //     single-candidate non-ambiguous interfaces to producer_assignment_. Build ambiguities_
  //     from the snapshots captured in pass 1.
  //
  //   Transitively-blocked post-pass (only if any ambiguities): walk the analysis to find
  //     interfaces that are not derivable but have at least one potential producer whose inputs
  //     are all classified and at least one is ambiguous or transitively blocked. Iterates to a
  //     fixed point.
  //
  // **Why pass 1's snapshot is the maximum candidate set:**
  //
  //   Pass 2 has stricter input gating than pass 1 (it skips transmissions whose inputs are
  //   in ambiguous_membership_). Therefore the set of firing transmissions in pass 2 is a
  //   SUBSET of the set firing in pass 1. Therefore for every interface, the candidate count
  //   in pass 2 is ≤ the count in pass 1. If an interface has ≥2 candidates in pass 1, it
  //   may have ≥2 in pass 2 (still ambiguous) OR fewer (no longer ambiguous in pass 2 — but
  //   the user authored multiple producers, so we still report it as ambiguous, using pass 1's
  //   full snapshot). If an interface has 1 candidate in pass 1, it has 1 or 0 in pass 2 (and
  //   we classify it accordingly). If 0 in pass 1, 0 in pass 2.
  //
  //   So pass 2 cannot discover NEW ambiguities. All ambiguities are detected in pass 1, and
  //   pass 2 just propagates the cascade. Termination is guaranteed in 2 passes max.
  //
  // **Per-phase complexity:**
  //
  //   Pass 1 / Pass 2 inner loop:    O(d × (T × m + G × s))  per pass
  //   Classification + snapshot:     O(N + K_amb × max_candidates)
  //   Final commit:                  O(N)
  //   Blocked post-pass (if any):    O(d × (T × m + N × s))
  //
  // **Total per analyze() call:**
  //
  //   Happy path (no ambiguities): O(d × (T × m + G × s) + N)
  //   Failing path (with ambiguities): roughly 3× the happy-path cost
  //
  //   For a typical robot arm (d~5, T~20, m~2, G~5, s~4, N~50): a few hundred ops.
  //   For a humanoid (d~10, T~200, m~4, G~30, s~6, N~500): tens of thousands of ops.
  //   Either way, microseconds at most.
  //
  // **Memory:** O(N + T) words.
  //
  // **Cycles in the analysis:** Forward/inverse transmission pairs (T_fwd: motor → joint and
  // T_inv: joint → motor) are explicitly supported. The dependency graph isn't a DAG, but the
  // 2-pass forward dataflow handles cycles via input-wins: only one direction's outputs are
  // consumed at any given runtime, and the other direction's "redundant" firings are filtered
  // out by `plan_joint_map`'s back-walk from requested outputs. The inner loop terminates
  // because `derivable_membership_` only grows during a single pass.
  // ===========================================================================

  const std::size_t state_count = analysis_.state_interface_order().inverse.size();

  // ---- Initialize the cross-pass state ------------------------------------
  ambiguous_membership_.assign(state_count, false);
  ambiguities_.clear();
  transitively_blocked_interfaces_.clear();
  transitively_blocked_membership_.assign(state_count, false);
  unproducible_interfaces_.clear();

  // Local accumulators (re-built each pass).
  std::unordered_map<StateInterfaceId, std::vector<StateInterfaceProducer>> candidates;
  // Captured at first detection — preserves the FULL candidate list from pass 1, so the
  // user-visible AmbiguousInterface report shows everything the user authored, even producers
  // that don't fire in the pass-2 restricted run.
  std::unordered_map<StateInterfaceId, std::vector<StateInterfaceProducer>> ambiguity_snapshots;

  // ---- Per-pass helpers ---------------------------------------------------
  auto reset_per_pass = [&]() {
    derivable_interfaces_.clear();
    derivable_membership_.assign(state_count, false);
    producer_assignment_.clear();
    candidates.clear();
    redundant_equivalent_inputs_.clear();
  };

  auto seed_inputs = [&]() {
    for (std::size_t i = 0; i < inputs.size(); ++i) {
      const StateInterfaceId sid = inputs[i];
      // Out-of-range SIDs indicate a caller bug (using a stale id from a different
      // analysis, or a fabricated id). The builder layer (DefaultJointMapBuilder) catches
      // these up front and reports `Kind::UnknownInterface`. Direct callers of `analyze()`
      // get a debug assertion to catch the bug in test suites; release builds skip
      // silently to avoid crashing on a recoverable input.
      assert(sid < state_count && "TransmissionReachability::analyze: input SID out of range");
      if (sid >= state_count) {
        continue;
      }
      if (producer_assignment_.find(sid) == producer_assignment_.end()) {
        producer_assignment_[sid] = producers::Input{i};
        add_to_derivable(sid);
      }
      // else: duplicate, the first-occurrence Input producer wins
    }
  };

  // The inner loop: forward fixed point honoring the current `ambiguous_membership_`.
  // Iterates until `derivable_membership_` stops growing within a single pass.
  // Per-iteration cost: O(T × m + G × s). Iterations: ≤ d.
  auto run_inner_loop = [&]() {
    bool changed = true;
    while (changed) {
      changed = false;

      // ---- Transmission hyper-nodes ----
      // For each transmission whose inputs are all derivable AND not ambiguous, record it as
      // a candidate producer for each output. Add the output to derivable iff it isn't
      // already known ambiguous from a prior pass.
      for (TransmissionInstanceId tid = 0; tid < analysis_.transmissions().size(); ++tid) {
        const auto & instance = analysis_.transmissions()[tid];
        bool viable = true;
        for (const StateInterfaceId in : instance.input_ids) {
          if (in >= state_count || !derivable_membership_[in] || ambiguous_membership_[in]) {
            viable = false;
            break;
          }
        }
        if (!viable) {
          continue;
        }
        for (const StateInterfaceId out : instance.output_ids) {
          if (out >= state_count) continue;
          // Dedup against existing identical candidates from earlier inner-loop iterations.
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
            record_candidate(out, producers::Transmission{tid}, candidates);
          }
          if (!ambiguous_membership_[out]) {
            if (add_to_derivable(out)) {
              changed = true;
            }
          }
        }
      }

      // ---- Affine hyper-nodes ----
      // For each `(group, interface_id)` pair reachable from a currently-derivable member,
      // process the hyper-node: pick the lowest-JointId leaf source and project to other
      // members. The processed_groups dedup ensures each pair is processed at most once per
      // inner iteration.
      const auto derivable_snapshot = derivable_interfaces_;
      std::set<std::pair<JointId, std::size_t>> processed_groups;
      for (const StateInterfaceId sid : derivable_snapshot) {
        const auto & defn = resolve_state_interface(analysis_, sid);
        const AffineProjectionRule * rule = analysis_.affine_projection_rule(defn.interface_id);
        if (rule == nullptr) {
          continue;
        }
        const JointId root = analysis_.affine_root_of(defn.joint_id);
        const auto group_members = analysis_.affine_group_members(root);
        if (group_members.size() <= 1) {
          continue;
        }
        if (!processed_groups.insert({root, defn.interface_id.hash}).second) {
          continue;
        }
        process_affine_hypernode(
          defn.interface_id, *rule, group_members, candidates, changed);
      }
    }
  };

  // ===========================================================================
  // Pass 1: unrestricted discovery
  // ===========================================================================
  reset_per_pass();
  seed_inputs();
  run_inner_loop();

  // ---- Classify and snapshot first detections ----------------------------
  // Walk the candidates map. Any interface with ≥2 entries is newly ambiguous; capture its
  // full candidate list as the snapshot. This snapshot survives across the pass-2 reset.
  bool any_ambiguous = false;
  for (const auto & kv : candidates) {
    if (kv.second.size() >= 2 && !ambiguous_membership_[kv.first]) {
      ambiguous_membership_[kv.first] = true;
      ambiguity_snapshots[kv.first] = kv.second;  // copy
      any_ambiguous = true;
    }
  }

  // ===========================================================================
  // Pass 2: restricted (only if pass 1 found any ambiguities)
  // ===========================================================================
  if (any_ambiguous) {
    reset_per_pass();
    seed_inputs();
    run_inner_loop();
  }
  // After pass 2 (or pass 1 if it was the only one): the candidates map reflects the final,
  // ambiguity-honest state. derivable_membership_ contains exactly the interfaces that have
  // a unique computable producer chain. ambiguous_membership_ contains exactly the interfaces
  // that the user authored multiple producers for. The two sets are disjoint by construction:
  //   - The inner loop's add_to_derivable check is gated on `!ambiguous_membership_[out]`
  //   - The transmission viability check skips transmissions whose inputs are ambiguous
  // Therefore an ambiguous interface is never in derivable_membership_ at the end.

  // ---- Final commit -------------------------------------------------------
  // For each non-ambiguous candidate with exactly 1 producer, commit it. We never commit
  // an ambiguous interface (it stays as monostate in producer_of()).
  for (auto & kv : candidates) {
    if (kv.second.empty()) {
      continue;
    }
    if (ambiguous_membership_[kv.first]) {
      continue;  // ambiguous interfaces never get committed to producer_assignment_
    }
    if (kv.second.size() == 1) {
      if (producer_assignment_.find(kv.first) == producer_assignment_.end()) {
        producer_assignment_[kv.first] = std::move(kv.second.front());
      }
    }
    // kv.second.size() >= 2 cannot happen here for a non-ambiguous interface (by the
    // monotonic-shrinking argument: if it had ≥2 candidates in pass 2, it would have had ≥2
    // in pass 1 too, and would have been flagged ambiguous in the snapshot phase above).
  }

  // ---- Build ambiguities_ from the pass-1 snapshots -----------------------
  for (auto & kv : ambiguity_snapshots) {
    AmbiguousInterface entry{};
    entry.interface = kv.first;
    entry.candidates = std::move(kv.second);
    ambiguities_.push_back(std::move(entry));
  }

  // ===========================================================================
  // Transitively-blocked post-pass: classify "non-derivable, non-ambiguous" interfaces into
  // {transitively_blocked, unproducible}. Only runs if there are any ambiguities.
  // ===========================================================================
  if (!ambiguities_.empty()) {
    run_transitively_blocked_post_pass(state_count);
  }

  // ---- Compute unproducible_interfaces_ ------------------------------------
  // Everything in the analysis that's not in derivable, ambiguous, or transitively_blocked is
  // unproducible. O(N) walk; runs once per analyze() call.
  for (StateInterfaceId i = 0; i < state_count; ++i) {
    if (derivable_membership_[i]) continue;
    if (ambiguous_membership_[i]) continue;
    if (transitively_blocked_membership_[i]) continue;
    unproducible_interfaces_.push_back(i);
  }
}

void TransmissionReachability::run_transitively_blocked_post_pass(const std::size_t state_count)
{
  // An interface is TRANSITIVELY BLOCKED iff:
  //   - it is not derivable
  //   - it is not ambiguous
  //   - at least one transmission could produce it whose every input is in
  //     (derivable ∪ ambiguous ∪ transitively_blocked) AND at least one input is in
  //     (ambiguous ∪ transitively_blocked)
  //
  // OR (for affine projections):
  //   - it has a non-trivial affine group with a registered rule, no clean leaf source exists
  //     in the group, and at least one group member is in (ambiguous ∪ transitively_blocked)
  //
  // Iterates to a fixed point. Per-iteration: O(T × m + B × s) where B is the number of
  // potentially-affine-blockable interfaces (precomputed once below). Iterations: ≤ d.

  // ---- Pre-compute the affine-blockable interface list -------------------
  // Only state interfaces that are BOTH (a) in a non-trivial affine group AND (b) have a
  // registered projection rule for their interface_id are candidates for affine-driven
  // blocking. Computing this once avoids walking every state interface inside the inner
  // fixed-point loop.
  std::vector<StateInterfaceId> affine_blockable_sids;
  for (StateInterfaceId sid = 0; sid < state_count; ++sid) {
    const auto & defn = analysis_.state_interface_order().inverse[sid];
    if (analysis_.affine_projection_rule(defn.interface_id) == nullptr) continue;
    const JointId root = analysis_.affine_root_of(defn.joint_id);
    if (analysis_.affine_group_members(root).size() <= 1) continue;
    affine_blockable_sids.push_back(sid);
  }

  bool blocked_changed = true;
  while (blocked_changed) {
    blocked_changed = false;

    // ---- Transmission-driven blocking ----
    for (TransmissionInstanceId tid = 0; tid < analysis_.transmissions().size(); ++tid) {
      const auto & instance = analysis_.transmissions()[tid];
      bool any_input_problematic = false;
      bool all_inputs_classifiable = true;
      for (const StateInterfaceId in : instance.input_ids) {
        if (in >= state_count) {
          all_inputs_classifiable = false;
          break;
        }
        const bool d = derivable_membership_[in];
        const bool a = ambiguous_membership_[in];
        const bool b = transitively_blocked_membership_[in];
        if (!d && !a && !b) {
          // Genuinely unproducible input — this transmission cannot serve as a "blocker chain"
          // for its outputs (the outputs are unproducible, not transitively blocked).
          all_inputs_classifiable = false;
          break;
        }
        if (a || b) {
          any_input_problematic = true;
        }
      }
      if (!all_inputs_classifiable || !any_input_problematic) {
        continue;
      }
      for (const StateInterfaceId out : instance.output_ids) {
        if (out >= state_count) continue;
        if (derivable_membership_[out]) continue;  // already derivable via another path
        if (ambiguous_membership_[out]) continue;
        if (transitively_blocked_membership_[out]) continue;
        transitively_blocked_membership_[out] = true;
        transitively_blocked_interfaces_.push_back(out);
        blocked_changed = true;
      }
    }

    // ---- Affine projection-driven blocking ----
    // Walk the precomputed list of affine-blockable interfaces only. For each non-derivable,
    // non-ambiguous, non-transitively-blocked entry: walk the group looking for any clean leaf
    // source. If the only available leaf sources are problematic, mark transitively blocked.
    for (const StateInterfaceId sid : affine_blockable_sids) {
      if (derivable_membership_[sid]) continue;
      if (ambiguous_membership_[sid]) continue;
      if (transitively_blocked_membership_[sid]) continue;
      const auto & defn = analysis_.state_interface_order().inverse[sid];
      const JointId root = analysis_.affine_root_of(defn.joint_id);
      const auto group_members = analysis_.affine_group_members(root);
      bool any_problematic_leaf = false;
      bool any_clean_leaf = false;
      for (const JointId member : group_members) {
        const auto member_def = StateInterfaceDefinition{member, defn.interface_id};
        if (!analysis_.state_interface_order().contains_key(member_def)) continue;
        const StateInterfaceId member_sid = analysis_.state_interface_order()[member_def];
        if (member_sid >= state_count) continue;
        if (derivable_membership_[member_sid]) {
          any_clean_leaf = true;
          break;  // a clean leaf exists; sid would be derivable, not transitively blocked
        }
        if (ambiguous_membership_[member_sid] || transitively_blocked_membership_[member_sid]) {
          any_problematic_leaf = true;
        }
      }
      if (!any_clean_leaf && any_problematic_leaf) {
        transitively_blocked_membership_[sid] = true;
        transitively_blocked_interfaces_.push_back(sid);
        blocked_changed = true;
      }
    }
  }
}

}  // namespace arm_kinematics
