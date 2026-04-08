//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/transmission_analysis.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <utility>

namespace arm_kinematics {

namespace {

// Built-in default projection rules. Effort is intentionally NOT registered by default —
// URDF mimic joints are kinematic constraints (typically enforced by control logic, not by
// physical gears or belts), so energy-conserving effort propagation would be a strong assumption
// that could silently produce wrong joint torques. Users who actually have a physical coupling
// must opt in by calling set_affine_projection_rule("effort", ...).
void populate_default_projection_rules(std::unordered_map<InterfaceId, AffineProjectionRule> & registry)
{
  // Position: q_b = m·q_a + o
  registry.emplace(InterfaceId{"position"}, AffineProjectionRule{1.0F, 1.0F, false});
  // Velocity: v_b = m·v_a   (offset drops under d/dt)
  registry.emplace(InterfaceId{"velocity"}, AffineProjectionRule{1.0F, 0.0F, false});
  // Acceleration: a_b = m·a_a
  registry.emplace(InterfaceId{"acceleration"}, AffineProjectionRule{1.0F, 0.0F, false});
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
    state_interface_order_(other.state_interface_order_),
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
  state_interface_order_ = other.state_interface_order_;
  projection_rules_ = other.projection_rules_;
  producers_index_ = other.producers_index_;
  affine_parent_ = other.affine_parent_;
  affine_group_members_storage_ = other.affine_group_members_storage_;
  return *this;
}

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
    // New joint: initialize as its own affine root with a singleton member list.
    affine_parent_.push_back(id);
    affine_group_members_storage_.push_back({id});
  }
  return id;
}

StateInterfaceId TransmissionAnalysis::ensure_state_interface_id(const StateInterfaceDefinition & definition)
{
  // Precondition: the joint id inside the definition must already be present in joint_order_.
  // The NamedStateInterfaceDefinition overload guarantees this by calling ensure_joint_id first;
  // callers of the raw StateInterfaceDefinition overload are responsible for the same.
  assert(definition.joint_id < joint_order_.inverse.size() &&
         "ensure_state_interface_id: definition.joint_id is not present in joint_order() — "
         "did you obtain it from ensure_joint_id?");

  const bool was_present = state_interface_order_.contains_key(definition);
  const StateInterfaceId id = state_interface_order_.ensure(definition);
  if (!was_present) {
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
  const float multiplier,
  const float offset)
{
  if (multiplier == 0.0F) {
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

  affine_transmissions_.push_back(AffineTransmission{
    source_joint_id,
    target_joint_id,
    multiplier,
    offset
  });

  // Maintain the union-find affine group index. Both joint ids are guaranteed to have
  // entries in affine_parent_/affine_group_members_storage_ because the joint_count check
  // above implies they were added via ensure_joint_id.
  const JointId root_a = affine_find(source_joint_id);
  const JointId root_b = affine_find(target_joint_id);
  if (root_a != root_b) {
    // Pick the smaller-id root as the new root for stability of root-joint identity.
    const JointId winner = std::min(root_a, root_b);
    const JointId loser = std::max(root_a, root_b);
    affine_parent_[loser] = winner;

    // Migrate the loser's group members into the winner's list.
    auto & winner_members = affine_group_members_storage_[winner];
    auto & loser_members = affine_group_members_storage_[loser];
    winner_members.insert(winner_members.end(), loser_members.begin(), loser_members.end());
    loser_members.clear();
    // Note: deliberately not calling shrink_to_fit() — typical group sizes are tiny and
    // releasing the small allocation isn't worth the allocator churn.
  }
}

void TransmissionAnalysis::add_affine_transmission(
  const std::string & source_joint_name,
  const std::string & target_joint_name,
  const float multiplier,
  const float offset)
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
  // Precondition: `j` is a valid JointId previously returned by ensure_joint_id, so
  // affine_parent_[j] is guaranteed to exist by invariant.
  assert(j < affine_parent_.size() && "affine_root_of: JointId out of range — was it returned by ensure_joint_id?");
  return affine_find(j);
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

JointId TransmissionAnalysis::affine_find(JointId j) const noexcept
{
  // Iterative find with path compression.
  JointId root = j;
  while (affine_parent_[root] != root) {
    root = affine_parent_[root];
  }
  // Path compression: make every node on the path point directly at the root.
  while (affine_parent_[j] != root) {
    const JointId next = affine_parent_[j];
    affine_parent_[j] = root;
    j = next;
  }
  return root;
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
