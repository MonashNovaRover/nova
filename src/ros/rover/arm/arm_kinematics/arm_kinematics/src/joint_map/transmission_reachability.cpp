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
  derivable_interfaces_.clear();
  derivable_membership_.assign(analysis_->state_interface_order().inverse.size(), false);
  producer_assignment_.clear();
  ambiguities_.clear();
  redundant_equivalent_inputs_.clear();

  const auto add_to_derivable = [this](const StateInterfaceId sid) {
    if (sid >= derivable_membership_.size()) {
      // Out of range — interface not in the analysis. Skip silently.
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
  // Phase 1: Initialize from inputs, with first-occurrence-wins.
  // ---------------------------------------------------------------------------
  for (std::size_t i = 0; i < inputs.size(); ++i) {
    const StateInterfaceId sid = inputs[i];
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
    for (TransmissionInstanceId tid = 0; tid < analysis_->transmissions().size(); ++tid) {
      const auto & instance = analysis_->transmissions()[tid];
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
    // Dedup (group root, interface id hash) processing within this iteration.
    std::set<std::pair<JointId, std::size_t>> processed_groups;
    for (const StateInterfaceId sid : derivable_snapshot) {
      const auto & defn = resolve_state_interface(*analysis_, sid);
      const AffineProjectionRule * rule = analysis_->affine_projection_rule(defn.interface_id);
      if (rule == nullptr) {
        continue;  // No projection rule for this interface id — affine doesn't propagate.
      }
      const JointId root = analysis_->affine_root_of(defn.joint_id);
      const auto group_members = analysis_->affine_group_members(root);
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
      //       doc warns about.
      JointId source_joint = std::numeric_limits<JointId>::max();
      bool found_leaf_source = false;
      // Track all leaf-Input candidates so we can populate redundant_equivalent_inputs_.
      std::vector<JointId> leaf_input_joints_in_group;
      for (const JointId member : group_members) {
        const auto member_def = StateInterfaceDefinition{member, defn.interface_id};
        if (!analysis_->state_interface_order().contains_key(member_def)) {
          continue;
        }
        const StateInterfaceId member_sid = analysis_->state_interface_order()[member_def];
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
        continue;
      }

      // Record redundant_equivalent_inputs: every leaf-Input joint in the group OTHER than
      // source_joint counts as a redundant equivalent input.
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

      // Get the source's StateInterfaceId.
      const auto source_def = StateInterfaceDefinition{source_joint, defn.interface_id};
      const StateInterfaceId source_sid = analysis_->state_interface_order()[source_def];

      // For each non-source member of the group, create an AffineProjection candidate.
      for (const JointId member : group_members) {
        if (member == source_joint) continue;
        const auto member_def = StateInterfaceDefinition{member, defn.interface_id};
        if (!analysis_->state_interface_order().contains_key(member_def)) {
          continue;
        }
        const StateInterfaceId member_sid = analysis_->state_interface_order()[member_def];
        const auto coeffs = compute_affine_projection_coefficients(source_joint, member, *rule);
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
      ambiguities_.push_back(std::move(entry));
      // Ambiguous interfaces have NO entry in producer_assignment_ — producer_of() returns
      // monostate for them.
    }
  }
}

}  // namespace arm_kinematics
