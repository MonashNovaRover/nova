//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/transmission_analysis.hpp"

#include <algorithm>
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
  models_.reserve(other.models_.size());
  for (const auto & model : other.models_) {
    models_.push_back(model ? model->clone() : nullptr);
  }
}

TransmissionAnalysis & TransmissionAnalysis::operator=(const TransmissionAnalysis & other)
{
  if (this == &other) {
    return *this;
  }

  models_.clear();
  models_.reserve(other.models_.size());
  for (const auto & model : other.models_) {
    models_.push_back(model ? model->clone() : nullptr);
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
  const JointId id = joint_order_.ensure(name);
  ensure_affine_group_capacity_for_all_joints();
  return id;
}

StateInterfaceId TransmissionAnalysis::ensure_state_interface_id(const StateInterfaceDefinition & definition)
{
  const StateInterfaceId id = state_interface_order_.ensure(definition);
  ensure_producers_index_capacity_for_all_state_interfaces();
  return id;
}

// ---------------------------------------------------------------------------
// add_transmission
// ---------------------------------------------------------------------------

void TransmissionAnalysis::add_transmission(
  const TransmissionModelId model_id,
  std::vector<StateInterfaceId> && inputs,
  std::vector<StateInterfaceId> && outputs,
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

  // Snapshot output ids before the move so we can update the inverse index after.
  const std::vector<StateInterfaceId> outputs_for_index = outputs;

  transmissions_.push_back(TransmissionInstance{
    model_id,
    std::move(inputs),
    std::move(outputs),
    std::move(name)
  });

  // Maintain the inverse index. Duplicates in outputs_for_index are intentionally allowed
  // (per spec) — but we should still only register the instance once per output to keep
  // the index minimal. Use a small dedup set.
  ensure_producers_index_capacity_for_all_state_interfaces();
  for (size_t i = 0; i < outputs_for_index.size(); ++i) {
    const StateInterfaceId sid = outputs_for_index[i];
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
    target_joint_id,
    source_joint_id,
    multiplier,
    offset
  });

  // Maintain the union-find affine group index.
  ensure_affine_group_capacity_for_all_joints();
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
    loser_members.shrink_to_fit();
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
  projection_rules_[std::move(interface_id)] = rule;
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
  if (j >= affine_parent_.size()) {
    // Joint isn't tracked in the union-find yet (e.g. it was added directly to joint_order_
    // without going through ensure_joint_id). Treat it as its own trivial root.
    return j;
  }
  return affine_find(j);
}

span<const JointId> TransmissionAnalysis::affine_group_members(const JointId root) const noexcept
{
  if (root >= affine_group_members_storage_.size()) {
    return {};
  }
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
  if (state_interface_id >= producers_index_.size()) {
    return {};
  }
  return producers_index_[state_interface_id];
}

// ---------------------------------------------------------------------------
// Capacity helpers
// ---------------------------------------------------------------------------

void TransmissionAnalysis::ensure_affine_group_capacity_for_all_joints()
{
  const auto joint_count = joint_order_.inverse.size();
  const auto current = affine_parent_.size();
  if (current >= joint_count) {
    return;
  }
  affine_parent_.resize(joint_count);
  affine_group_members_storage_.resize(joint_count);
  // Initialize new joints as their own root with a singleton member list.
  for (auto j = current; j < joint_count; ++j) {
    affine_parent_[j] = static_cast<JointId>(j);
    affine_group_members_storage_[j].push_back(static_cast<JointId>(j));
  }
}

void TransmissionAnalysis::ensure_producers_index_capacity_for_all_state_interfaces()
{
  const auto sid_count = state_interface_order_.inverse.size();
  if (producers_index_.size() < sid_count) {
    producers_index_.resize(sid_count);
  }
}

} // namespace arm_kinematics
