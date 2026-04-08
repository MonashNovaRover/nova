//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_ANALYSIS_HPP
#define ARM_KINEMATICS_TRANSMISSION_ANALYSIS_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "arm_kinematics/joint_map/affine_projection_rule.hpp"
#include "arm_kinematics/joint_map/state_interface_definition.hpp"
#include "arm_kinematics/joint_map/transmission_model.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/interface_id.hpp"
#include "arm_kinematics/utilities/order.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * Build-time graph data structure for transmissions and joint relationships.
 *
 * Holds:
 * - The set of joints (named, with stable internal `JointId`s)
 * - The set of state interfaces (joint + interface type, with stable `StateInterfaceId`s)
 * - `TransmissionModel`s and `TransmissionInstance`s (state-interface-edged)
 * - `AffineTransmission`s (joint-edged — represent mimic-like relationships)
 * - The `InterfaceId → AffineProjectionRule` registry (with safe defaults for position, velocity,
 *   acceleration; effort is opt-in only)
 * - An inverse index from `StateInterfaceId → producing TransmissionInstanceIds`
 * - A union-find affine group index keyed by joint
 *
 * `TransmissionAnalysis` is **append-only** — there is no remove API. The inverse and affine-group
 * indices are maintained incrementally as `add_transmission` / `add_affine_transmission` are called.
 *
 * `TransmissionAnalysis` knows nothing about URDF, ros2_control, or mimic joints. It is purely a
 * typed graph of joints, state interfaces, and the relationships between them.
 *
 * \note **Not thread-safe.** Even read-only queries may mutate internal state (`affine_root_of`
 * performs path compression). All access must be externally synchronized.
 *
 * \note Mutating operations (`add_transmission`, `add_affine_transmission`, `ensure_*`) are not
 * strong-exception-safe: if an internal index update throws (e.g. OOM), partial state is left in
 * place. Realistically OOM in setup code is fatal anyway.
 */
class ARM_KINEMATICS_PUBLIC TransmissionAnalysis {
public:
  struct AffineTransmission {
    /// The joint read by this affine transmission.
    JointId source_joint_id = 0;
    /// The joint written to by this affine transmission.
    JointId target_joint_id = 0;
    /// Must be non-zero (validated on add). m=0 would model `target = offset` (a constant) which
    /// isn't really a mimic relationship and breaks bidirectional affine-group semantics.
    float multiplier = 1.0F;
    float offset = 0.0F;
  };

  struct TransmissionInstance {
    TransmissionModelId model_id = 0;
    std::vector<StateInterfaceId> input_ids;
    std::vector<StateInterfaceId> output_ids;

    /// Only used for logging / error messages — gives users a meaningful way to identify the
    /// transmission in builder errors ("transmission `differential_left` is ambiguous with ...").
    std::string name;
  };

  TransmissionAnalysis();
  TransmissionAnalysis(const TransmissionAnalysis & other);
  TransmissionAnalysis(TransmissionAnalysis &&) noexcept = default;
  TransmissionAnalysis & operator=(const TransmissionAnalysis & other);
  TransmissionAnalysis & operator=(TransmissionAnalysis &&) noexcept = default;
  ~TransmissionAnalysis() = default;

  // ---------------------------------------------------------------------------
  // Models
  // ---------------------------------------------------------------------------

  [[nodiscard]] const std::vector<std::unique_ptr<TransmissionModel>> & models() const noexcept
  {
    return models_;
  }

  TransmissionModelId add_model(std::unique_ptr<TransmissionModel> model);

  // ---------------------------------------------------------------------------
  // Transmissions
  // ---------------------------------------------------------------------------

  [[nodiscard]] const std::vector<TransmissionInstance> & transmissions() const noexcept
  {
    return transmissions_;
  }

  /// a.k.a. mimic joints, or any joint-level affine relationship.
  [[nodiscard]] const std::vector<AffineTransmission> & affine_transmissions() const noexcept
  {
    return affine_transmissions_;
  }

  // ---------------------------------------------------------------------------
  // Orderings
  // ---------------------------------------------------------------------------

  /// Canonical boundary mapping from named joints to stable internal `JointId`s.
  [[nodiscard]] const Order<std::string, JointId> & joint_order() const noexcept
  {
    return joint_order_;
  }

  /// Canonical mapping from `(JointId, InterfaceId)` to stable internal `StateInterfaceId`s.
  [[nodiscard]] const Order<StateInterfaceDefinition, StateInterfaceId> & state_interface_order() const noexcept
  {
    return state_interface_order_;
  }

  /// Provides the JointId from joint_order_, adding it to the end of the order if it is not already present.
  JointId ensure_joint_id(const std::string & name);

  StateInterfaceId ensure_state_interface_id(const StateInterfaceDefinition & definition);
  StateInterfaceId ensure_state_interface_id(const NamedStateInterfaceDefinition & definition)
  {
    return ensure_state_interface_id(StateInterfaceDefinition{
      ensure_joint_id(definition.joint_name),
      definition.interface_id
    });
  }

  // ---------------------------------------------------------------------------
  // add_transmission
  // ---------------------------------------------------------------------------

  /// Adds a transmission instance to the analysis. Parameters are taken by value to support
  /// both lvalue and rvalue arguments uniformly; the function moves them into storage.
  void add_transmission(
    TransmissionModelId model_id,
    std::vector<StateInterfaceId> inputs,
    std::vector<StateInterfaceId> outputs,
    std::string name = "unnamed");

  // ---------------------------------------------------------------------------
  // add_affine_transmission
  // ---------------------------------------------------------------------------

  /**
   * Adds one joint-level affine transmission (mimic) to the analysis.
   *
   * \warning This API does not validate cycles between affine transmissions across multiple edges.
   * Callers must not add cyclic affine transmission relationships.
   *
   * \pre `multiplier != 0`. Zero multipliers don't represent real mimic relationships and break
   * bidirectional affine-group semantics. Throws `std::invalid_argument` on violation.
   * \pre `source_joint_id != target_joint_id`. Self-loops are degenerate (they collapse to a
   * constant or a contradiction depending on multiplier) and are always a user error. Throws
   * `std::invalid_argument` on violation.
   */
  void add_affine_transmission(
    JointId source_joint_id,
    JointId target_joint_id,
    float multiplier = 1.0F,
    float offset = 0.0F);

  /// Convenience overload — resolves joint names first.
  void add_affine_transmission(
    const std::string & source_joint_name,
    const std::string & target_joint_name,
    float multiplier = 1.0F,
    float offset = 0.0F);

  // ---------------------------------------------------------------------------
  // Affine projection rule registry
  // ---------------------------------------------------------------------------

  /// Sets (or replaces) the projection rule for a given interface id. After this, affine
  /// transmissions will project onto the specified interface using the given rule.
  void set_affine_projection_rule(InterfaceId interface_id, AffineProjectionRule rule);

  /// Returns a pointer to the rule for the given interface id, or `nullptr` if no rule is registered.
  /// An interface with no registered rule does not participate in affine projection at all.
  [[nodiscard]] const AffineProjectionRule * affine_projection_rule(const InterfaceId & interface_id) const noexcept;

  // ---------------------------------------------------------------------------
  // Affine group queries (union-find)
  // ---------------------------------------------------------------------------

  /// Returns the root joint of the affine group containing `j`. If `j` has no affine relationships,
  /// returns `j` itself (every joint is in a group, possibly trivial).
  ///
  /// \pre `j` is a valid `JointId` previously returned by `ensure_joint_id` (debug-asserted).
  [[nodiscard]] JointId affine_root_of(JointId j) const noexcept;

  /// All members of the affine group whose root is `root`. Always contains at least `root`.
  /// The returned span is invalidated by any subsequent `add_affine_transmission` call.
  ///
  /// \pre `root` must actually be a root joint, as returned by `affine_root_of` (debug-asserted).
  /// Passing a non-root joint is a precondition violation; the result is unspecified in release
  /// builds and asserts in debug builds. Use `affine_group_members(affine_root_of(j))` if you only
  /// have an arbitrary member.
  [[nodiscard]] span<const JointId> affine_group_members(JointId root) const noexcept;

  // ---------------------------------------------------------------------------
  // Inverse transmission index
  // ---------------------------------------------------------------------------

  /// Returns the list of TransmissionInstanceIds whose `output_ids` contain the given state interface.
  /// Built incrementally as `add_transmission` is called. Lets the subgraph algorithm answer
  /// "who produces X?" in expected O(1).
  ///
  /// The returned span is invalidated by any subsequent `add_transmission` call.
  ///
  /// \pre `state_interface_id` is a valid id previously returned by `ensure_state_interface_id`
  /// (debug-asserted).
  [[nodiscard]] span<const TransmissionInstanceId> producing_transmissions(StateInterfaceId state_interface_id) const noexcept;

private:
  // Owned data
  std::vector<std::unique_ptr<TransmissionModel>> models_{};
  std::vector<AffineTransmission> affine_transmissions_{};
  std::vector<TransmissionInstance> transmissions_{};

  /// Joint name → JointId
  Order<std::string, JointId> joint_order_{};
  /// (JointId, InterfaceId) → StateInterfaceId
  Order<StateInterfaceDefinition, StateInterfaceId> state_interface_order_{};

  /// InterfaceId → projection rule. Populated with sane defaults for position, velocity, and
  /// acceleration in the constructor. Effort is *not* registered by default.
  std::unordered_map<InterfaceId, AffineProjectionRule> projection_rules_{};

  /// Inverse transmission index: state_interface_id → list of TransmissionInstanceIds whose
  /// `output_ids` contain that state interface. Indexed by StateInterfaceId (dense).
  ///
  /// **Invariant:** `producers_index_.size() == state_interface_order_.inverse.size()`.
  /// Maintained atomically by `ensure_state_interface_id` (the only entry point that grows the
  /// state interface set). `add_transmission` then appends to the relevant per-output entries.
  std::vector<std::vector<TransmissionInstanceId>> producers_index_{};

  /// Union-find for affine groups, indexed by JointId.
  ///
  /// **Invariant:** `affine_parent_.size() == affine_group_members_storage_.size() == joint_order_.inverse.size()`.
  /// Maintained atomically by `ensure_joint_id` (the only entry point that grows the joint set).
  ///
  /// `affine_parent_[j]` is the parent of `j` in the union-find tree (with path compression).
  /// `affine_group_members_storage_[root]` is the materialized member list for that root; for
  /// non-root joints the entry is empty (members are migrated into the winning root on union).
  mutable std::vector<JointId> affine_parent_{};
  std::vector<std::vector<JointId>> affine_group_members_storage_{};

  /// Internal find with path compression.
  JointId affine_find(JointId j) const noexcept;
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_ANALYSIS_HPP
