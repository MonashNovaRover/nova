//
// Created by Bailey Chessum on 8/4/26.
//

#include "arm_kinematics/joint_map/transmission_reachability.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <unordered_map>
#include <utility>

namespace arm_kinematics {

using StateInterfaceId = TransmissionReachability::StateInterfaceId;
using InterfaceKindId = TransmissionAnalysis::InterfaceKindId;
using CanonicalStateInterfaceDefinition = TransmissionAnalysis::CanonicalStateInterfaceDefinition;

// File-local aliases so internal code can use the short names without re-polluting the
// public arm_kinematics:: namespace.
using producers::AmbiguousInterface;
using producers::StateInterfaceProducer;

namespace {

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
  const span<const StateInterfaceDefinition> inputs)
{
  return TransmissionReachability(analysis, inputs);
}

TransmissionReachability::TransmissionReachability(
  const TransmissionAnalysis & analysis,
  const span<const StateInterfaceDefinition> inputs)
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

span<const StateInterfaceDefinition> TransmissionReachability::inputs() const noexcept
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
  if (interface >= producer_assignment_.size() || !producer_assignment_[interface].has_value()) {
    return std::monostate{};
  }
  return *producer_assignment_[interface];
}

StateInterfaceProducer TransmissionReachability::producer_of_def(
  const StateInterfaceDefinition & def) const noexcept
{
  // Check bare-def producers first (populated during affine propagation for non-SID members).
  const auto def_it = def_producer_assignment_.find(def);
  if (def_it != def_producer_assignment_.end()) {
    return def_it->second;
  }
  // Fall through to the SID-indexed assignment for registered definitions.
  const auto opt_sid = analysis_.find_state_interface_id(def);
  if (opt_sid.has_value()) {
    return producer_of(*opt_sid);
  }
  return std::monostate{};
}

span<const StateInterfaceDefinition> TransmissionReachability::redundant_equivalent_inputs() const noexcept
{
  return redundant_equivalent_inputs_;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool TransmissionReachability::is_derivable_def(const StateInterfaceDefinition & def) const noexcept
{
  const auto opt_sid = analysis_.find_state_interface_id(def);
  if (opt_sid.has_value()) {
    const StateInterfaceId sid = *opt_sid;
    return sid < derivable_membership_.size() && derivable_membership_[sid];
  }
  return derivable_bare_defs_set_.count(def) > 0;
}

bool TransmissionReachability::add_to_derivable_bare_def(const StateInterfaceDefinition & def)
{
  if (!derivable_bare_defs_set_.insert(def).second) {
    return false;  // already derivable
  }
  derivable_bare_defs_list_.push_back(def);
  return true;
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
  const auto & t_src = analysis_.affine_transmission_of(source_joint);
  const auto & t_tgt = analysis_.affine_transmission_of(target_joint);
  assert(t_src.multiplier != 0.0F && "compute_affine_projection_coefficients: source joint's flat multiplier is zero");

  const double joint_m = t_tgt.multiplier / t_src.multiplier;
  const double joint_o = t_tgt.offset - joint_m * t_src.offset;

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
  std::vector<StateInterfaceProducer> & candidates_first,
  std::vector<std::vector<StateInterfaceProducer>> & candidates_extra) const
{
  if (iface < producer_assignment_.size() &&
      producer_assignment_[iface].has_value() &&
      is_input_producer(*producer_assignment_[iface]))
  {
    return;
  }
  if (std::holds_alternative<std::monostate>(candidates_first[iface])) {
    candidates_first[iface] = std::move(producer);
  } else {
    candidates_extra[iface].push_back(std::move(producer));
  }
}

void TransmissionReachability::process_affine_hypernode(
  const InterfaceKindId interface_kind_id,
  const AffineProjectionRule & rule,
  const span<const JointId> group_members,
  std::vector<StateInterfaceProducer> & candidates_first,
  std::vector<std::vector<StateInterfaceProducer>> & candidates_extra,
  bool & changed)
{
  // ---- Find the lowest-JointId leaf source for this hypernode -------------------------
  // A member is a "leaf source" candidate if it is derivable (via real SID or bare def) and:
  //   (a) has an Input or Transmission producer already committed, OR
  //   (b) has exactly one Transmission candidate (not yet committed)
  // This covers both registered (SID) and bare (no SID) group members uniformly.
  JointId source_joint = std::numeric_limits<JointId>::max();
  bool found_leaf_source = false;
  std::vector<JointId> leaf_input_joints_in_group;

  const InterfaceId & interface_id = analysis_.interface_order().inverse[interface_kind_id];

  for (const JointId member : group_members) {
    const auto member_def = StateInterfaceDefinition{member, interface_id};
    if (!is_derivable_def(member_def)) continue;

    // Ambiguity check (only meaningful for real SIDs — bare defs can't be ambiguous).
    const auto opt_sid = analysis_.find_state_interface_id(CanonicalStateInterfaceDefinition{
      member,
      interface_kind_id
    });
    if (opt_sid.has_value() && ambiguous_membership_[*opt_sid]) continue;

    bool is_leaf = false;
    bool is_input_leaf = false;

    if (opt_sid.has_value()) {
      const StateInterfaceId member_sid = *opt_sid;
      const bool has_assigned_producer =
        member_sid < producer_assignment_.size() && producer_assignment_[member_sid].has_value();
      if (has_assigned_producer && is_leaf_producer(*producer_assignment_[member_sid])) {
        is_leaf = true;
        is_input_leaf = is_input_producer(*producer_assignment_[member_sid]);
      } else if (!has_assigned_producer) {
        // A SID with exactly 1 Transmission candidate (in primary, no overflow) is a leaf.
        if (!std::holds_alternative<std::monostate>(candidates_first[member_sid]) &&
            candidates_extra[member_sid].empty() &&
            std::holds_alternative<producers::Transmission>(candidates_first[member_sid]))
        {
          is_leaf = true;
        }
      }
    } else {
      // Bare def: only Input producers are possible (bare defs can't come from transmissions).
      const auto def_it = def_producer_assignment_.find(member_def);
      if (def_it != def_producer_assignment_.end() && is_leaf_producer(def_it->second)) {
        is_leaf = true;
        is_input_leaf = is_input_producer(def_it->second);
      }
    }

    if (!is_leaf) continue;
    if (member < source_joint) {
      source_joint = member;
      found_leaf_source = true;
    }
    if (is_input_leaf) {
      leaf_input_joints_in_group.push_back(member);
    }
  }
  if (!found_leaf_source) return;

  // ---- Record redundant equivalent inputs ------------------------------------------
  for (const JointId leaf_joint : leaf_input_joints_in_group) {
    if (leaf_joint == source_joint) continue;
    const auto leaf_def = StateInterfaceDefinition{leaf_joint, interface_id};
    if (std::find(redundant_equivalent_inputs_.begin(),
                  redundant_equivalent_inputs_.end(),
                  leaf_def) == redundant_equivalent_inputs_.end())
    {
      redundant_equivalent_inputs_.push_back(leaf_def);
    }
  }

  // ---- Project from source to every other group member ----------------------------
  const StateInterfaceDefinition source_def{source_joint, interface_id};

  for (const JointId member : group_members) {
    if (member == source_joint) continue;
    const auto member_def = StateInterfaceDefinition{member, interface_id};
    const auto coeffs = compute_affine_projection_coefficients(source_joint, member, rule);
    producers::AffineProjection projection{};
    projection.source = source_def;   // StateInterfaceDefinition, not SID
    projection.multiplier = coeffs.multiplier;
    projection.offset = coeffs.offset;

    const auto opt_member_sid = analysis_.find_state_interface_id(CanonicalStateInterfaceDefinition{
      member,
      interface_kind_id
    });
    if (opt_member_sid.has_value()) {
      // Registered member: go through candidates for proper ambiguity detection.
      const StateInterfaceId member_sid = *opt_member_sid;
      bool already_listed = false;
      if (!std::holds_alternative<std::monostate>(candidates_first[member_sid])) {
        if (const auto * ap = std::get_if<producers::AffineProjection>(&candidates_first[member_sid])) {
          if (ap->source == projection.source &&
              ap->multiplier == projection.multiplier &&
              ap->offset == projection.offset)
          {
            already_listed = true;
          }
        }
        if (!already_listed) {
          for (const auto & c : candidates_extra[member_sid]) {
            if (const auto * ap = std::get_if<producers::AffineProjection>(&c)) {
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
      }
      if (!already_listed) {
        record_candidate(member_sid, projection, candidates_first, candidates_extra);
      }
      if (!ambiguous_membership_[member_sid]) {
        if (add_to_derivable(member_sid)) {
          changed = true;
        }
      }
    } else {
      // Bare member: directly commit (bare defs can't be ambiguous — the hypernode already
      // picked the best leaf source, and bare defs have no competing transmission producers).
      const auto [it, inserted] = def_producer_assignment_.emplace(member_def, projection);
      if (inserted) {
        if (add_to_derivable_bare_def(member_def)) {
          changed = true;
        }
      }
    }
  }
}

void TransmissionReachability::run_fixed_point(const span<const StateInterfaceDefinition> inputs)
{
  // See the detailed algorithm overview in the previous version — the 2-pass structure,
  // complexity analysis, and cycle-handling rationale are unchanged. The key extension here
  // is that affine propagation now covers ALL joints in a group, not just those with a
  // registered StateInterfaceId. Bare inputs seed the fixed point; bare affine targets are
  // committed directly in def_producer_assignment_ without going through the candidates map.

  const std::size_t state_count = analysis_.state_interface_order().inverse.size();

  // ---- Initialize the cross-pass state ------------------------------------
  ambiguous_membership_.assign(state_count, false);
  ambiguities_.clear();
  ambiguities_.reserve(state_count);
  transitively_blocked_interfaces_.clear();
  transitively_blocked_interfaces_.reserve(state_count);
  transitively_blocked_membership_.assign(state_count, false);
  unproducible_interfaces_.clear();
  unproducible_interfaces_.reserve(state_count);
  producer_assignment_.assign(state_count, std::nullopt);

  // Flat primary storage for candidates: one slot per SID, no heap allocation per element.
  // monostate = no candidate recorded yet. For non-ambiguous topologies (the common case),
  // each SID gets at most 1 candidate, and the flat array is the sole allocation.
  // Overflow candidates for ambiguous SIDs (2+ viable producers) go in candidates_extra.
  std::vector<StateInterfaceProducer> candidates_first(state_count, std::monostate{});
  std::vector<std::vector<StateInterfaceProducer>> candidates_extra(state_count);
  std::vector<std::vector<StateInterfaceProducer>> ambiguity_snapshots(state_count);

  // ---- Per-pass helpers ---------------------------------------------------
  auto reset_per_pass = [&]() {
    derivable_interfaces_.clear();
    derivable_interfaces_.reserve(state_count);
    derivable_membership_.assign(state_count, false);
    producer_assignment_.assign(state_count, std::nullopt);
    std::fill(candidates_first.begin(), candidates_first.end(), std::monostate{});
    for (auto & v : candidates_extra) {
      v.clear();
    }
    redundant_equivalent_inputs_.clear();
    redundant_equivalent_inputs_.reserve(inputs.size());
    // Bare-def tracking must also be reset between passes.
    def_producer_assignment_.clear();
    def_producer_assignment_.reserve(inputs.size());
    derivable_bare_defs_set_.clear();
    derivable_bare_defs_set_.reserve(inputs.size());
    derivable_bare_defs_list_.clear();
    derivable_bare_defs_list_.reserve(inputs.size());
  };

  auto seed_inputs = [&]() {
    for (std::size_t i = 0; i < inputs.size(); ++i) {
      const StateInterfaceDefinition & def = inputs[i];
      const auto opt_kind_id = analysis_.find_interface_kind_id(def.interface_id);
      const auto opt_sid = opt_kind_id.has_value()
        ? analysis_.find_state_interface_id(CanonicalStateInterfaceDefinition{def.joint_id, *opt_kind_id})
        : std::nullopt;
      if (opt_sid.has_value()) {
        // Registered definition: use the SID-indexed path.
        const StateInterfaceId sid = *opt_sid;
        if (!producer_assignment_[sid].has_value()) {
          producer_assignment_[sid] = producers::Input{i};
          add_to_derivable(sid);
        }
        // else: duplicate, first-occurrence Input producer wins
      } else {
        // Bare definition (no registered SID): seed directly in def_producer_assignment_.
        const auto [it, inserted] = def_producer_assignment_.emplace(def, producers::Input{i});
        if (inserted) {
          add_to_derivable_bare_def(def);
        }
        // else: duplicate bare input, first-occurrence wins
      }
    }
  };

  // The inner loop: forward fixed point honoring the current `ambiguous_membership_`.
  // Allocate the affine group deduplication bitmap once per run_inner_loop call
  // (keyed by root * num_interface_kinds + interface_kind_id) and reset it each
  // while(changed) iteration. This avoids the per-iteration std::set tree-node
  // allocations that the previous version incurred.
  const std::size_t num_joints = analysis_.joint_order().inverse.size();
  const std::size_t num_kinds = analysis_.interface_order().inverse.size();

  // Check once whether any non-trivial affine group exists. Joints with no affine
  // transmission have affine_root_of(j) == j. If all joints are self-roots, the affine
  // scanning block in run_inner_loop can be skipped entirely, avoiding 2*num_joints
  // string hash lookups per fixed-point iteration (one affine_projection_rule call per
  // derivable SID that always returns nullptr or finds size-1 groups).
  const bool has_non_trivial_affine_groups = [&]() {
    for (JointId j = 0; j < static_cast<JointId>(num_joints); ++j) {
      if (analysis_.affine_root_of(j) != j) {
        return true;
      }
    }
    return false;
  }();

  auto run_inner_loop = [&]() {
    std::vector<bool> processed_groups(num_joints * num_kinds, false);
    bool changed = true;
    while (changed) {
      changed = false;

      // ---- Transmission hyper-nodes ----
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
          bool already_listed = false;
          if (!std::holds_alternative<std::monostate>(candidates_first[out])) {
            if (const auto * tx = std::get_if<producers::Transmission>(&candidates_first[out])) {
              if (tx->instance_id == tid) already_listed = true;
            }
            if (!already_listed) {
              for (const auto & c : candidates_extra[out]) {
                if (const auto * tx = std::get_if<producers::Transmission>(&c)) {
                  if (tx->instance_id == tid) { already_listed = true; break; }
                }
              }
            }
          }
          if (!already_listed) {
            record_candidate(out, producers::Transmission{tid}, candidates_first, candidates_extra);
          }
          if (!ambiguous_membership_[out]) {
            if (add_to_derivable(out)) {
              changed = true;
            }
          }
        }
      }

      // ---- Affine hyper-nodes ----
      // Only run affine scanning when the analysis actually has non-trivial affine groups.
      // For pure-transmission or pure-input topologies (e.g. LinearChain, InputReorder),
      // this block is skipped entirely, avoiding per-SID string hash lookups on every
      // fixed-point iteration.
      if (has_non_trivial_affine_groups) {
      // Reset the per-iteration deduplication bitmap.
      std::fill(processed_groups.begin(), processed_groups.end(), false);

      auto process_affine_defn = [&](const StateInterfaceDefinition & defn) {
        const AffineProjectionRule * rule = analysis_.affine_projection_rule(defn.interface_id);
        if (rule == nullptr) {
          return;
        }
        const auto opt_kind_id = analysis_.find_interface_kind_id(defn.interface_id);
        if (!opt_kind_id.has_value()) {
          return;
        }
        const JointId root = analysis_.affine_root_of(defn.joint_id);
        const auto group_members = analysis_.affine_group_members(root);
        if (group_members.size() <= 1) {
          return;
        }
        const std::size_t group_key = root * num_kinds + *opt_kind_id;
        if (processed_groups[group_key]) {
          return;
        }
        processed_groups[group_key] = true;
        process_affine_hypernode(
          *opt_kind_id, *rule, group_members, candidates_first, candidates_extra, changed);
      };

      for (const StateInterfaceId sid : derivable_interfaces_) {
        process_affine_defn(analysis_.state_interface_order().inverse[sid]);
      }
      for (const StateInterfaceDefinition & defn : derivable_bare_defs_list_) {
        process_affine_defn(defn);
      }
      }  // has_non_trivial_affine_groups
    }
  };

  // ===========================================================================
  // Pass 1: unrestricted discovery
  // ===========================================================================
  reset_per_pass();
  seed_inputs();
  run_inner_loop();

  // ---- Classify and snapshot first detections ----------------------------
  bool any_ambiguous = false;
  for (StateInterfaceId sid = 0; sid < state_count; ++sid) {
    // An SID is ambiguous iff it has 2+ candidates: candidates_first is non-monostate AND
    // at least one overflow candidate exists.
    if (!candidates_extra[sid].empty() && !ambiguous_membership_[sid]) {
      ambiguous_membership_[sid] = true;
      ambiguity_snapshots[sid].push_back(candidates_first[sid]);
      ambiguity_snapshots[sid].insert(
        ambiguity_snapshots[sid].end(),
        candidates_extra[sid].begin(),
        candidates_extra[sid].end());
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

  // ---- Final commit -------------------------------------------------------
  for (StateInterfaceId sid = 0; sid < state_count; ++sid) {
    if (std::holds_alternative<std::monostate>(candidates_first[sid])) {
      continue;
    }
    if (ambiguous_membership_[sid]) {
      continue;
    }
    if (!producer_assignment_[sid].has_value()) {
      producer_assignment_[sid] = std::move(candidates_first[sid]);
    }
  }

  // ---- Build ambiguities_ from the pass-1 snapshots -----------------------
  for (StateInterfaceId sid = 0; sid < state_count; ++sid) {
    if (ambiguity_snapshots[sid].empty()) {
      continue;
    }
    AmbiguousInterface entry{};
    entry.interface = analysis_.state_interface_order().inverse[sid];
    entry.candidates = std::move(ambiguity_snapshots[sid]);
    ambiguities_.push_back(std::move(entry));
  }

  // ===========================================================================
  // Transitively-blocked post-pass
  // ===========================================================================
  if (!ambiguities_.empty()) {
    run_transitively_blocked_post_pass(state_count);
  }

  // ---- Compute unproducible_interfaces_ (real SIDs only) -------------------
  // Bare defs that are unproducible are handled by diagnose_missing_outputs via
  // producer_of_def() — they simply return monostate and the caller classifies them.
  for (StateInterfaceId i = 0; i < state_count; ++i) {
    if (derivable_membership_[i]) continue;
    if (ambiguous_membership_[i]) continue;
    if (transitively_blocked_membership_[i]) continue;
    unproducible_interfaces_.push_back(i);
  }
}

void TransmissionReachability::run_transitively_blocked_post_pass(const std::size_t state_count)
{
  std::vector<StateInterfaceId> affine_blockable_sids;
  affine_blockable_sids.reserve(state_count);
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
        if (derivable_membership_[out]) continue;
        if (ambiguous_membership_[out]) continue;
        if (transitively_blocked_membership_[out]) continue;
        transitively_blocked_membership_[out] = true;
        transitively_blocked_interfaces_.push_back(out);
        blocked_changed = true;
      }
    }

    // ---- Affine projection-driven blocking ----
    for (const StateInterfaceId sid : affine_blockable_sids) {
      if (derivable_membership_[sid]) continue;
      if (ambiguous_membership_[sid]) continue;
      if (transitively_blocked_membership_[sid]) continue;
      const auto & defn = analysis_.state_interface_order().inverse[sid];
      const auto opt_kind_id = analysis_.find_interface_kind_id(defn.interface_id);
      if (!opt_kind_id.has_value()) continue;
      const JointId root = analysis_.affine_root_of(defn.joint_id);
      const auto group_members = analysis_.affine_group_members(root);
      bool any_problematic_leaf = false;
      bool any_clean_leaf = false;
      const InterfaceId & interface_id = analysis_.interface_order().inverse[*opt_kind_id];
      for (const JointId member : group_members) {
        const auto member_def = StateInterfaceDefinition{member, interface_id};
        const auto opt_member_sid = analysis_.find_state_interface_id(CanonicalStateInterfaceDefinition{
          member,
          *opt_kind_id
        });
        if (!opt_member_sid.has_value()) {
          // Bare member: check bare derivability (clean leaf if derivable, not problematic).
          if (derivable_bare_defs_set_.count(member_def) > 0) {
            any_clean_leaf = true;
            break;
          }
          continue;
        }
        const StateInterfaceId member_sid = *opt_member_sid;
        if (member_sid >= state_count) continue;
        if (derivable_membership_[member_sid]) {
          any_clean_leaf = true;
          break;
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
