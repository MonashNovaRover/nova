//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/transmission_analysis.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <utility>

#include "arm_kinematics/joint_map/transmission_model.hpp"

namespace arm_kinematics {

using StateInterfaceId = TransmissionAnalysis::StateInterfaceId;
using InterfaceKindId = TransmissionAnalysis::InterfaceKindId;
using CanonicalStateInterfaceDefinition = TransmissionAnalysis::CanonicalStateInterfaceDefinition;

namespace {

// Built-in default projection rules. Effort is intentionally NOT registered by default —
// URDF mimic joints are kinematic constraints (typically enforced by control logic, not by
// physical gears or belts), so energy-conserving effort propagation would be a strong assumption
// that could silently produce wrong joint torques. Users who actually have a physical coupling
// must opt in by calling set_affine_projection_rule("effort", ...).
void populate_default_projection_rules(std::unordered_map<InterfaceId, AffineProjectionRule> & registry)
{
  // Position: q_b = m·q_a + o
  registry.emplace(InterfaceId::Position(), AffineProjectionRule{1.0, 1.0, false});
  // Velocity: v_b = m·v_a   (offset drops under d/dt)
  registry.emplace(InterfaceId::Velocity(), AffineProjectionRule{1.0, 0.0, false});
  // Acceleration: a_b = m·a_a
  registry.emplace(InterfaceId::Acceleration(), AffineProjectionRule{1.0, 0.0, false});
}

// Tolerance for floating-point equality checks on composed affine coefficients. Used in the
// redundant-edge consistency assertion in `add_affine_transmission`. Hybrid relative + absolute:
// values within 1e-5 absolute are considered equal (handles near-zero); larger values are
// compared with ~5 digits of relative agreement (handles arbitrary scale without false positives
// from FP rounding accumulated through chained compositions).
constexpr double kAffineCoefficientTolerance = 1.0e-9;

bool affine_coefficients_approx_equal(const double a, const double b) noexcept
{
  const double diff = std::fabs(a - b);
  if (diff <= kAffineCoefficientTolerance) {
    return true;
  }
  const double largest = std::max(std::fabs(a), std::fabs(b));
  return diff <= kAffineCoefficientTolerance * largest;
}

}  // namespace

TransmissionAnalysis::TransmissionAnalysis()
{
  populate_default_projection_rules(projection_rules_);
}

TransmissionAnalysis::TransmissionAnalysis(const TransmissionAnalysis & other)
  : affine_transmissions_(other.affine_transmissions_),
    transmissions_(other.transmissions_),
    joint_order_(other.joint_order_),
    interface_order_(other.interface_order_),
    state_interface_order_(other.state_interface_order_),
    canonical_state_interface_order_(other.canonical_state_interface_order_),
    projection_rules_(other.projection_rules_),
    producers_index_(other.producers_index_),
    affine_parent_(other.affine_parent_),
    affine_group_members_storage_(other.affine_group_members_storage_)
{
  // add_model() rejects nullptr, so models_ never holds nulls.
  models_.reserve(other.models_.size());
  for (const auto & model : other.models_) {
    models_.push_back(model->clone());
  }
}

TransmissionAnalysis::TransmissionAnalysis(TransmissionAnalysis &&) noexcept = default;

TransmissionAnalysis & TransmissionAnalysis::operator=(const TransmissionAnalysis & other)
{
  if (this == &other) {
    return *this;
  }

  // add_model() rejects nullptr, so models_ never holds nulls.
  models_.clear();
  models_.reserve(other.models_.size());
  for (const auto & model : other.models_) {
    models_.push_back(model->clone());
  }

  affine_transmissions_ = other.affine_transmissions_;
  transmissions_ = other.transmissions_;
  joint_order_ = other.joint_order_;
  interface_order_ = other.interface_order_;
  state_interface_order_ = other.state_interface_order_;
  canonical_state_interface_order_ = other.canonical_state_interface_order_;
  projection_rules_ = other.projection_rules_;
  producers_index_ = other.producers_index_;
  affine_parent_ = other.affine_parent_;
  affine_group_members_storage_ = other.affine_group_members_storage_;
  return *this;
}

TransmissionAnalysis & TransmissionAnalysis::operator=(TransmissionAnalysis &&) noexcept = default;

TransmissionAnalysis::~TransmissionAnalysis() = default;

// ---------------------------------------------------------------------------
// Models
// ---------------------------------------------------------------------------

TransmissionModelId TransmissionAnalysis::add_model(std::unique_ptr<TransmissionModel> model)
{
  if (!model) {
    throw std::invalid_argument("TransmissionAnalysis::add_model() received a null model");
  }

  const auto model_id = models_.size();
  models_.push_back(std::move(model));
  return model_id;
}

// ---------------------------------------------------------------------------
// Orderings
// ---------------------------------------------------------------------------

JointId TransmissionAnalysis::ensure_joint_id(const std::string & name)
{
  // Detect insertion vs. existing-key lookup, so we can grow the per-joint side data
  // atomically with the ordering. Order::ensure is "look up or insert" — without the
  // pre-check we'd have to scan to find what's new, which leaks the invariant.
  const bool was_present = joint_order_.contains_key(name);
  const JointId id = joint_order_.ensure(name);
  if (!was_present) {
    // New joint: initialize as its own affine root with a singleton member list and an
    // identity affine transmission entry (j = 1*j + 0).
    affine_parent_.push_back(id);
    affine_group_members_storage_.push_back({id});
    affine_transmissions_.push_back(AffineTransmission{
      /*source_joint_id=*/id,
      /*target_joint_id=*/id,
      /*multiplier=*/1.0,
      /*offset=*/0.0,
    });
  }
  return id;
}

std::optional<StateInterfaceId> TransmissionAnalysis::find_state_interface_id(
  const StateInterfaceDefinition & definition) const noexcept
{
  const auto opt_kind_id = find_interface_kind_id(definition.interface_id);
  if (!opt_kind_id.has_value()) {
    return std::nullopt;
  }
  return find_state_interface_id(CanonicalStateInterfaceDefinition{
    definition.joint_id,
    *opt_kind_id
  });
}

std::optional<StateInterfaceId> TransmissionAnalysis::find_state_interface_id(
  const CanonicalStateInterfaceDefinition & definition) const noexcept
{
  if (!canonical_state_interface_order_.contains_key(definition)) {
    return std::nullopt;
  }
  return canonical_state_interface_order_[definition];
}

std::optional<InterfaceKindId> TransmissionAnalysis::find_interface_kind_id(
  const InterfaceId & interface_id) const noexcept
{
  if (!interface_order_.contains_key(interface_id)) {
    return std::nullopt;
  }
  return interface_order_[interface_id];
}

StateInterfaceId TransmissionAnalysis::ensure_state_interface_id(const StateInterfaceDefinition & definition)
{
  // Precondition: the joint id inside the definition must already be present in joint_order_.
  // The NamedStateInterfaceDefinition overload guarantees this by calling ensure_joint_id first;
  // callers of the raw StateInterfaceDefinition overload are responsible for the same.
  assert(definition.joint_id < joint_order_.inverse.size() &&
         "ensure_state_interface_id: definition.joint_id is not present in joint_order() — "
         "did you obtain it from ensure_joint_id?");

  const InterfaceKindId interface_kind_id = interface_order_.ensure(definition.interface_id);
  const CanonicalStateInterfaceDefinition canonical_definition{
    definition.joint_id,
    interface_kind_id
  };

  const bool was_present = canonical_state_interface_order_.contains_key(canonical_definition);
  const StateInterfaceId id = canonical_state_interface_order_.ensure(canonical_definition);
  if (!was_present) {
    state_interface_order_.ensure(definition, id);
    // New state interface: empty producer list until some transmission writes to it.
    producers_index_.emplace_back();
  }
  return id;
}

// ---------------------------------------------------------------------------
// add_transmission
// ---------------------------------------------------------------------------

void TransmissionAnalysis::add_transmission(
  const TransmissionModelId model_id,
  std::vector<StateInterfaceId> inputs,
  std::vector<StateInterfaceId> outputs,
  std::string name)
{
  if (model_id >= models_.size()) {
    throw std::invalid_argument(
      "TransmissionAnalysis::add_transmission() received a model_id that is not present in models()");
  }

  const auto state_interface_count = state_interface_order_.inverse.size();
  for (const auto sid : inputs) {
    if (sid >= state_interface_count) {
      throw std::invalid_argument(
        "TransmissionAnalysis::add_transmission() received an input StateInterfaceId not present in "
        "state_interface_order()");
    }
  }
  for (const auto sid : outputs) {
    if (sid >= state_interface_count) {
      throw std::invalid_argument(
        "TransmissionAnalysis::add_transmission() received an output StateInterfaceId not present in "
        "state_interface_order()");
    }
  }

  const TransmissionInstanceId instance_id = transmissions_.size();

  transmissions_.push_back(TransmissionInstance{
    model_id,
    std::move(inputs),
    std::move(outputs),
    std::move(name)
  });

  // Maintain the inverse index. The producers_index_ is guaranteed sized to match
  // state_interface_order_ by ensure_state_interface_id; we just append to per-output
  // lists here. Duplicate ids in output_ids are intentionally allowed (per spec) — but
  // we still only register the instance once per output to keep the index minimal.
  // Iterate over the just-pushed instance's output_ids directly to avoid copying.
  const auto & new_outputs = transmissions_.back().output_ids;
  for (const StateInterfaceId sid : new_outputs) {
    auto & producers = producers_index_[sid];
    if (std::find(producers.begin(), producers.end(), instance_id) == producers.end()) {
      producers.push_back(instance_id);
    }
  }
}

// ---------------------------------------------------------------------------
// add_affine_transmission
// ---------------------------------------------------------------------------

void TransmissionAnalysis::add_affine_transmission(
  const JointId source_joint_id,
  const JointId target_joint_id,
  const double multiplier,
  const double offset)
{
  if (multiplier == 0.0) {
    throw std::invalid_argument(
      "TransmissionAnalysis::add_affine_transmission() received multiplier == 0. Zero multipliers "
      "do not represent real mimic relationships and break bidirectional affine-group semantics. "
      "Joints that are always at a constant value should be supplied as inputs directly or via the "
      "default-value-source mechanism.");
  }

  if (source_joint_id == target_joint_id) {
    throw std::invalid_argument(
      "TransmissionAnalysis::add_affine_transmission() received source_joint_id == target_joint_id. "
      "Self-loops are degenerate (they collapse to a constant for m != 1, or a contradiction for "
      "m == 1, o != 0) and are always a user error.");
  }

  const auto joint_count = joint_order_.inverse.size();
  if (source_joint_id >= joint_count) {
    throw std::invalid_argument(
      "TransmissionAnalysis::add_affine_transmission() received a source JointId not present in joint_order()");
  }
  if (target_joint_id >= joint_count) {
    throw std::invalid_argument(
      "TransmissionAnalysis::add_affine_transmission() received a target JointId not present in joint_order()");
  }

  // The new edge says: target_value = multiplier * source_value + offset.
  //
  // Both source_joint_id and target_joint_id already have entries in affine_transmissions_
  // (initialized to identity by ensure_joint_id). We compose the new edge into the existing
  // flat relations and update everything that needs to change so that the per-joint flat
  // invariant holds after the call.
  //
  // **Directional semantic:** the canonical "root" of an affine group is the source-side end
  // of the chain — the joint everything else ultimately derives from. The source's current
  // root therefore always wins the merge.

  const AffineTransmission t_source = affine_transmissions_[source_joint_id];
  const AffineTransmission t_target = affine_transmissions_[target_joint_id];

  // source's current root and target's current root.
  const JointId winner = t_source.source_joint_id;  // == affine_parent_[source_joint_id]
  const JointId loser = t_target.source_joint_id;   // == affine_parent_[target_joint_id]

  // target's NEW flat from the winner root, computed by substituting source's old flat
  // into the new edge equation:
  //   target = m * source + o
  //          = m * (t_source.m * winner + t_source.o) + o
  //          = (m * t_source.m) * winner + (m * t_source.o + o)
  const double new_m = multiplier * t_source.multiplier;
  const double new_o = multiplier * t_source.offset + offset;

  if (winner == loser) {
    // The new edge is redundant — source and target are already in the same affine group.
    // In a consistent setup the new (m, o) should match target's existing flat (t_target.m,
    // t_target.o), within FP-rounding tolerance accumulated through chained compositions. If
    // they disagree beyond tolerance the user has added inconsistent edges (a precondition
    // violation per the header's warning); we debug-assert but otherwise leave the existing
    // entry in place — modifying it would silently let the latest call override the prior
    // analysis, which is worse than leaving it inconsistent.
    assert(affine_coefficients_approx_equal(new_m, t_target.multiplier) &&
           affine_coefficients_approx_equal(new_o, t_target.offset) &&
           "add_affine_transmission: inconsistent redundant edge — caller has added "
           "contradictory affine relationships within the same group (precondition violation).");
    return;
  }

  // Different roots — merge the loser's group into the winner's.
  //
  // We need every member of the loser's old group (currently expressed as
  // `member = m_member * loser + o_member`) to be re-expressed in terms of the winner.
  //
  // First, derive the loser-root-to-winner-root transformation. We have two ways to
  // express target:
  //   - new flat:    target = new_m * winner + new_o
  //   - old flat:    target = t_target.m * loser + t_target.o
  // Equating and solving for loser:
  //   loser = (new_m / t_target.m) * winner + ((new_o - t_target.o) / t_target.m)
  //
  // By induction `t_target.multiplier` is always non-zero in well-behaved use: identity
  // entries start at 1.0, and every subsequent composition is a product of non-zero edge
  // multipliers (the `multiplier == 0` check above) and non-zero existing multipliers. The
  // only way to land at exactly zero here is FP underflow from a chain of pathologically
  // small multipliers — a degenerate input the user is responsible for. We debug-assert
  // against it so the corruption is loud rather than silent.
  assert(t_target.multiplier != 0.0 &&
         "add_affine_transmission: target's stored composed multiplier is zero — likely "
         "FP underflow from a chain of pathologically small affine multipliers. The analysis "
         "would be corrupted by the divide-by-zero below.");
  const double root_to_winner_m = new_m / t_target.multiplier;
  const double root_to_winner_o = (new_o - t_target.offset) / t_target.multiplier;

  // Now walk every member of the loser's group, updating its stored flat relation.
  // For each member j with old flat `j = m_j * loser + o_j`, substituting the loser's
  // new expression:
  //   j = m_j * (root_to_winner_m * winner + root_to_winner_o) + o_j
  //     = (m_j * root_to_winner_m) * winner + (m_j * root_to_winner_o + o_j)
  // This loop also re-points each member's parent to the winner and migrates it into
  // the winner's member list — same single pass as the union does.
  auto & winner_members = affine_group_members_storage_[winner];
  auto & loser_members = affine_group_members_storage_[loser];
  winner_members.reserve(winner_members.size() + loser_members.size());
  for (const JointId member : loser_members) {
    auto & relation = affine_transmissions_[member];
    const double old_m = relation.multiplier;
    const double old_o = relation.offset;
    relation.source_joint_id = winner;
    relation.multiplier = old_m * root_to_winner_m;
    relation.offset = old_m * root_to_winner_o + old_o;

    affine_parent_[member] = winner;
    winner_members.push_back(member);
  }
  loser_members.clear();
  // Note: deliberately not calling shrink_to_fit() — typical group sizes are tiny and
  // releasing the small allocation isn't worth the allocator churn.
}

void TransmissionAnalysis::add_affine_transmission(
  const std::string & source_joint_name,
  const std::string & target_joint_name,
  const double multiplier,
  const double offset)
{
  add_affine_transmission(
    ensure_joint_id(source_joint_name),
    ensure_joint_id(target_joint_name),
    multiplier,
    offset);
}

// ---------------------------------------------------------------------------
// Affine projection rules
// ---------------------------------------------------------------------------

void TransmissionAnalysis::set_affine_projection_rule(InterfaceId interface_id, AffineProjectionRule rule)
{
  projection_rules_[std::move(interface_id)] = std::move(rule);
}

const AffineProjectionRule * TransmissionAnalysis::affine_projection_rule(const InterfaceId & interface_id) const noexcept
{
  const auto it = projection_rules_.find(interface_id);
  if (it == projection_rules_.end()) {
    return nullptr;
  }
  return &it->second;
}

// ---------------------------------------------------------------------------
// Affine group queries
// ---------------------------------------------------------------------------

JointId TransmissionAnalysis::affine_root_of(const JointId j) const noexcept
{
  // O(1) single lookup. The tree is kept maximally flat by eager union: every joint's parent
  // always points directly at its root. No walking, no path compression, no mutation.
  assert(j < affine_parent_.size() && "affine_root_of: JointId out of range — was it returned by ensure_joint_id?");
  return affine_parent_[j];
}

const TransmissionAnalysis::AffineTransmission & TransmissionAnalysis::affine_transmission_of(const JointId j) const noexcept
{
  // O(1) lookup. Per the storage invariant, each joint has exactly one entry indexed by joint id;
  // root joints have the identity entry, non-root joints have their composed flat relation.
  assert(j < affine_transmissions_.size() && "affine_transmission_of: JointId out of range — was it returned by ensure_joint_id?");
  return affine_transmissions_[j];
}

span<const JointId> TransmissionAnalysis::affine_group_members(const JointId root) const noexcept
{
  // Preconditions: `root` is a valid JointId AND it is actually the root of its affine group.
  // Passing a non-root member silently returns an empty span (the members live on the root entry,
  // not on each member entry), so we assert in debug builds. Use affine_root_of(j) first if you
  // only have an arbitrary group member.
  assert(root < affine_group_members_storage_.size() && "affine_group_members: JointId out of range");
  assert(affine_parent_[root] == root && "affine_group_members: argument is not a root joint — call affine_root_of() first");
  return affine_group_members_storage_[root];
}

// ---------------------------------------------------------------------------
// Inverse transmission index
// ---------------------------------------------------------------------------

span<const TransmissionInstanceId> TransmissionAnalysis::producing_transmissions(const StateInterfaceId state_interface_id) const noexcept
{
  // Precondition: `state_interface_id` is a valid id previously returned by
  // ensure_state_interface_id, so producers_index_[state_interface_id] is guaranteed
  // to exist by invariant.
  assert(state_interface_id < producers_index_.size() && "producing_transmissions: StateInterfaceId out of range — was it returned by ensure_state_interface_id?");
  return producers_index_[state_interface_id];
}

} // namespace arm_kinematics
