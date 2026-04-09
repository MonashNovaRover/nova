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
  TransmissionReachability result{};
  result.analysis_ = &analysis;
  result.inputs_.assign(inputs.begin(), inputs.end());
  result.run_fixed_point(inputs);
  return result;
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

span<const StateInterfaceId> TransmissionReachability::blocked_interfaces() const noexcept
{
  return blocked_interfaces_;
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
  const auto & t_src = analysis_->affine_transmission_of(source_joint);
  const auto & t_tgt = analysis_->affine_transmission_of(target_joint);
  assert(t_src.multiplier != 0.0F && "compute_affine_projection_coefficients: source joint's flat multiplier is zero");

  const float joint_m = t_tgt.multiplier / t_src.multiplier;
  const float joint_o = t_tgt.offset - joint_m * t_src.offset;

  // Now apply the projection rule's interface-space transformation. See
  // affine_projection_rule.hpp for the formal definition; in short:
  //   reverse_direction == false → target_iface = (mscale*joint_m) * source_iface + (oscale*joint_o)
  //   reverse_direction == true  → invert so the projection still reads source → target.
  AffineProjectionCoefficients result{};
  const float scaled_m = rule.multiplier_scale * joint_m;
  const float scaled_o = rule.offset_scale * joint_o;
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

void TransmissionReachability::run_fixed_point(const span<const StateInterfaceId> inputs)
{
  // The algorithm runs an iterative outer loop that builds up a `ambiguous_membership_` set.
  // Each outer pass runs an inner forward fixed point that *honors* the current ambiguous set
  // — transmissions whose inputs include any ambiguous interface do not fire, and ambiguous
  // outputs are not added to derivable. After each inner pass, classification adds any
  // newly-discovered multi-candidate interfaces to ambiguous_membership_; the outer loop
  // re-runs from scratch with the augmented set. Termination is guaranteed because the
  // ambiguous set grows monotonically (bounded by the number of state interfaces); in
  // practice convergence is in 1 pass (no ambiguities) or 2 passes (initial discovery +
  // confirm).
  //
  // After convergence, a final post-pass walks the transmission graph to populate
  // `blocked_interfaces_` — interfaces whose every potential producer transitively depends on
  // an ambiguous (or blocked) input. Builders surface these as "blocked by upstream
  // ambiguity" rather than as plain unreachable.

  const std::size_t state_count = analysis_->state_interface_order().inverse.size();

  ambiguous_membership_.assign(state_count, false);
  ambiguities_.clear();
  blocked_interfaces_.clear();
  blocked_membership_.assign(state_count, false);

  // Local candidate accumulator — re-built each outer pass. The final pass's candidates feed
  // the post-loop classification commit.
  std::unordered_map<StateInterfaceId, std::vector<StateInterfaceProducer>> candidates;

  const auto add_to_derivable = [this](const StateInterfaceId sid) {
    if (sid >= derivable_membership_.size()) {
      return false;
    }
    if (derivable_membership_[sid]) {
      return false;
    }
    derivable_membership_[sid] = true;
    derivable_interfaces_.push_back(sid);
    return true;
  };

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

  while (true) {
    // ---- Reset per-pass state -----------------------------------------------
    derivable_interfaces_.clear();
    derivable_membership_.assign(state_count, false);
    producer_assignment_.clear();
    candidates.clear();
    redundant_equivalent_inputs_.clear();

    // ---- Phase 1: Initialize from inputs (first-occurrence-wins) -----------
    for (std::size_t i = 0; i < inputs.size(); ++i) {
      const StateInterfaceId sid = inputs[i];
      if (sid >= state_count) {
        continue;  // out-of-range; skip silently
      }
      if (producer_assignment_.find(sid) == producer_assignment_.end()) {
        producer_assignment_[sid] = producers::Input{i};
        add_to_derivable(sid);
      }
    }

    // ---- Phase 2: Inner fixed point ----------------------------------------
    bool changed = true;
    while (changed) {
      changed = false;

      // ---- Transmissions ----
      for (TransmissionInstanceId tid = 0; tid < analysis_->transmissions().size(); ++tid) {
        const auto & instance = analysis_->transmissions()[tid];
        // Ambiguity-honest viability check: every input must be derivable AND not ambiguous.
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
          // Avoid duplicate candidate entries if we revisit T in a later inner iteration.
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
          // Only add to derivable if NOT already known ambiguous from a prior outer pass.
          // This is the algorithmic fix: an ambiguous interface doesn't propagate
          // derivability, so downstream transmissions that depend on it won't fire either.
          if (!ambiguous_membership_[out]) {
            if (add_to_derivable(out)) {
              changed = true;
            }
          }
        }
      }

      // ---- Affine projections ----
      const auto derivable_snapshot = derivable_interfaces_;
      std::set<std::pair<JointId, std::size_t>> processed_groups;
      for (const StateInterfaceId sid : derivable_snapshot) {
        const auto & defn = resolve_state_interface(*analysis_, sid);
        const AffineProjectionRule * rule = analysis_->affine_projection_rule(defn.interface_id);
        if (rule == nullptr) {
          continue;
        }
        const JointId root = analysis_->affine_root_of(defn.joint_id);
        const auto group_members = analysis_->affine_group_members(root);
        if (group_members.size() <= 1) {
          continue;
        }
        if (!processed_groups.insert({root, defn.interface_id.hash}).second) {
          continue;
        }

        // Find the lowest-JointId leaf source in the group. A member is a valid leaf source
        // if it has an Input producer (committed in phase 1) or a unique Transmission
        // candidate in `candidates` — the latter lets affine projections chain off
        // transmission outputs in the same outer pass.
        JointId source_joint = std::numeric_limits<JointId>::max();
        bool found_leaf_source = false;
        std::vector<JointId> leaf_input_joints_in_group;
        for (const JointId member : group_members) {
          const auto member_def = StateInterfaceDefinition{member, defn.interface_id};
          if (!analysis_->state_interface_order().contains_key(member_def)) continue;
          const StateInterfaceId member_sid = analysis_->state_interface_order()[member_def];
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
        if (!found_leaf_source) continue;

        // Record redundant_equivalent_inputs.
        for (const JointId leaf_joint : leaf_input_joints_in_group) {
          if (leaf_joint == source_joint) continue;
          const auto leaf_def = StateInterfaceDefinition{leaf_joint, defn.interface_id};
          const StateInterfaceId leaf_sid = analysis_->state_interface_order()[leaf_def];
          if (std::find(redundant_equivalent_inputs_.begin(),
                        redundant_equivalent_inputs_.end(),
                        leaf_sid) == redundant_equivalent_inputs_.end())
          {
            redundant_equivalent_inputs_.push_back(leaf_sid);
          }
        }

        const auto source_def = StateInterfaceDefinition{source_joint, defn.interface_id};
        const StateInterfaceId source_sid = analysis_->state_interface_order()[source_def];

        for (const JointId member : group_members) {
          if (member == source_joint) continue;
          const auto member_def = StateInterfaceDefinition{member, defn.interface_id};
          if (!analysis_->state_interface_order().contains_key(member_def)) continue;
          const StateInterfaceId member_sid = analysis_->state_interface_order()[member_def];
          const auto coeffs = compute_affine_projection_coefficients(source_joint, member, *rule);
          producers::AffineProjection projection{};
          projection.source = source_sid;
          projection.multiplier = coeffs.multiplier;
          projection.offset = coeffs.offset;
          // Dedup
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
          if (!ambiguous_membership_[member_sid]) {
            if (add_to_derivable(member_sid)) {
              changed = true;
            }
          }
        }
      }
    }

    // ---- Phase 3: Detect newly-ambiguous interfaces -------------------------
    bool any_new_ambiguous = false;
    for (const auto & kv : candidates) {
      if (kv.second.size() >= 2 && !ambiguous_membership_[kv.first]) {
        ambiguous_membership_[kv.first] = true;
        any_new_ambiguous = true;
      }
    }

    if (!any_new_ambiguous) {
      break;  // Outer loop converged.
    }
    // Otherwise: re-run with the augmented ambiguous set.
  }

  // ---------------------------------------------------------------------------
  // Final commit: walk the converged candidates map and populate producer_assignment_ +
  // ambiguities_. Note that an interface flagged ambiguous in an earlier pass may have <2
  // candidates in the final pass (because blocking caused some producers to stop firing).
  // In that case, the ambiguity is "stale" and should be revoked — the interface is actually
  // blocked, not ambiguous, and the post-pass below will catch it.
  // ---------------------------------------------------------------------------
  for (auto & kv : candidates) {
    const StateInterfaceId iface = kv.first;
    auto & cand_list = kv.second;
    if (cand_list.empty()) {
      continue;
    }
    if (cand_list.size() == 1) {
      if (producer_assignment_.find(iface) == producer_assignment_.end()) {
        producer_assignment_[iface] = std::move(cand_list.front());
      }
    } else {
      AmbiguousInterface entry{};
      entry.interface = iface;
      entry.candidates = std::move(cand_list);
      ambiguities_.push_back(std::move(entry));
      // ambiguous_membership_[iface] is already true from the outer loop.
    }
  }

  // Revoke stale ambiguities: any interface in ambiguous_membership_ that did NOT make it into
  // ambiguities_ above had its candidate count drop below 2 in the final pass (because of a
  // cascade where blocking upstream killed some of its producers). Such interfaces are
  // actually blocked, not ambiguous.
  std::vector<bool> still_ambiguous(state_count, false);
  for (const auto & amb : ambiguities_) {
    still_ambiguous[amb.interface] = true;
  }
  for (StateInterfaceId i = 0; i < state_count; ++i) {
    if (ambiguous_membership_[i] && !still_ambiguous[i]) {
      ambiguous_membership_[i] = false;
    }
  }

  // ---------------------------------------------------------------------------
  // Phase 4: Blocked post-pass.
  //
  // An interface is blocked iff:
  //   - it is not derivable
  //   - it is not ambiguous
  //   - at least one transmission could produce it whose every input is in
  //     (derivable ∪ ambiguous ∪ blocked) and at least one input is in (ambiguous ∪ blocked)
  //
  // OR (for affine projections):
  //   - it has a non-trivial affine group with a registered rule, and the lowest-JointId
  //     leaf source available in the group is in (ambiguous ∪ blocked)
  //
  // Iterate to a fixed point. Only runs if any ambiguities were found.
  // ---------------------------------------------------------------------------
  if (!ambiguities_.empty()) {
    bool blocked_changed = true;
    while (blocked_changed) {
      blocked_changed = false;

      // Transmission-driven blocking
      for (TransmissionInstanceId tid = 0; tid < analysis_->transmissions().size(); ++tid) {
        const auto & instance = analysis_->transmissions()[tid];
        bool any_input_problematic = false;
        bool all_inputs_classifiable = true;
        for (const StateInterfaceId in : instance.input_ids) {
          if (in >= state_count) {
            all_inputs_classifiable = false;
            break;
          }
          const bool d = derivable_membership_[in];
          const bool a = ambiguous_membership_[in];
          const bool b = blocked_membership_[in];
          if (!d && !a && !b) {
            // Genuinely unreachable input — this transmission cannot serve as a "blocker
            // chain" for its outputs (the outputs are unreachable, not blocked).
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
          if (blocked_membership_[out]) continue;
          blocked_membership_[out] = true;
          blocked_interfaces_.push_back(out);
          blocked_changed = true;
        }
      }

      // Affine projection-driven blocking. If a non-trivial affine group's lowest-JointId
      // available leaf is in (ambiguous ∪ blocked), the other group members for that
      // interface are also blocked (assuming no other valid leaf source exists).
      //
      // For simplicity, we check: for each non-derivable, non-ambiguous, non-blocked group
      // member, walk the group looking for any leaf source. If the only leaf sources we find
      // are problematic, mark blocked.
      //
      // Note: this is conservative — it may miss some blocked-via-affine cases — but it
      // handles the common ones. Refinement is deferred.
      for (StateInterfaceId sid = 0; sid < state_count; ++sid) {
        if (derivable_membership_[sid]) continue;
        if (ambiguous_membership_[sid]) continue;
        if (blocked_membership_[sid]) continue;
        const auto & defn = analysis_->state_interface_order().inverse[sid];
        const AffineProjectionRule * rule = analysis_->affine_projection_rule(defn.interface_id);
        if (rule == nullptr) continue;
        const JointId root = analysis_->affine_root_of(defn.joint_id);
        const auto group_members = analysis_->affine_group_members(root);
        if (group_members.size() <= 1) continue;
        bool any_problematic_leaf = false;
        bool any_clean_leaf = false;
        for (const JointId member : group_members) {
          const auto member_def = StateInterfaceDefinition{member, defn.interface_id};
          if (!analysis_->state_interface_order().contains_key(member_def)) continue;
          const StateInterfaceId member_sid = analysis_->state_interface_order()[member_def];
          if (member_sid >= state_count) continue;
          if (derivable_membership_[member_sid]) {
            any_clean_leaf = true;
            break;  // a clean leaf exists; sid would be derivable, not blocked
          }
          if (ambiguous_membership_[member_sid] || blocked_membership_[member_sid]) {
            any_problematic_leaf = true;
          }
        }
        if (!any_clean_leaf && any_problematic_leaf) {
          blocked_membership_[sid] = true;
          blocked_interfaces_.push_back(sid);
          blocked_changed = true;
        }
      }
    }
  }
}

}  // namespace arm_kinematics
