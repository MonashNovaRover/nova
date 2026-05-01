//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_ANALYSIS_HPP
#define ARM_KINEMATICS_TRANSMISSION_ANALYSIS_HPP

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "arm_kinematics/joint_map/affine_projection_rule.hpp"
#include "arm_kinematics/joint_map/state_interface_definition.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/interface_id.hpp"
#include "arm_kinematics/utilities/order.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

class TransmissionModel;

using InterfaceKindId = std::size_t;
using ProjectionKindId = std::size_t;

struct CanonicalStateInterfaceDefinition {
  JointId joint_id = 0;
  InterfaceKindId interface_kind_id = 0;

  bool operator<(const CanonicalStateInterfaceDefinition & other) const noexcept
  {
    return joint_id < other.joint_id ||
           (joint_id == other.joint_id && interface_kind_id < other.interface_kind_id);
  }

  bool operator==(const CanonicalStateInterfaceDefinition & other) const noexcept
  {
    return joint_id == other.joint_id && interface_kind_id == other.interface_kind_id;
  }

  bool operator!=(const CanonicalStateInterfaceDefinition & other) const noexcept
  {
    return !(*this == other);
  }
};

/**
 * Build-time graph data structure for transmissions and joint relationships.
 *
 * Holds:
 * - The set of joints (named, with stable internal `JointId`s)
 * - The set of state interfaces (joint + interface type, with stable `StateInterfaceId`s)
 * - `TransmissionModel`s and `TransmissionInstance`s (state-interface-edged)
 * - The current flat affine relation of each joint to its affine-group root, stored per-joint
 *   as an `AffineTransmission` and accessed via `affine_transmission_of(j)`. New edges added via
 *   `add_affine_transmission` are composed eagerly into these flat relations at the time of the
 *   call (the loser group's member list is walked and every member's stored relation is updated).
 * - The `ProjectionKindId → AffineProjectionRule` registry (with safe defaults for position,
 *   velocity, acceleration; effort is opt-in only)
 * - An inverse index from `StateInterfaceId → producing TransmissionInstanceIds`
 * - An eagerly-flattened affine group index keyed by joint (every joint's parent always points
 *   directly at its current group root, so `affine_root_of` is an O(1) array lookup with no
 *   walking or path compression)
 *
 * `TransmissionAnalysis` is **append-only** — there is no remove API. The inverse and affine-group
 * indices, and the per-joint flat affine relations, are maintained incrementally as
 * `add_transmission` / `add_affine_transmission` are called.
 *
 * `TransmissionAnalysis` knows nothing about URDF, ros2_control, or mimic joints. It is purely a
 * typed graph of joints, state interfaces, and the relationships between them.
 *
 * \note Read-only queries are pure reads — no internal mutation, no path compression. The class
 * is therefore safe for concurrent reads from multiple threads as long as no thread is mutating.
 *
 * \note Mutating operations (`add_transmission`, `add_affine_transmission`, `ensure_*`) are not
 * strong-exception-safe: if an internal index update throws (e.g. OOM), partial state is left in
 * place. Realistically OOM in setup code is fatal anyway.
 */
class ARM_KINEMATICS_PUBLIC TransmissionAnalysis {
public:
  using InterfaceKindId = arm_kinematics::InterfaceKindId;
  using ProjectionKindId = arm_kinematics::ProjectionKindId;
  using StateInterfaceId = std::size_t;
  using CanonicalStateInterfaceDefinition = arm_kinematics::CanonicalStateInterfaceDefinition;

  /**
   * The current flat affine relationship of one joint to its affine group's root.
   *
   * Storage invariant: there is exactly one `AffineTransmission` per joint, indexed by `JointId`,
   * accessed via `affine_transmission_of(j)`. For each entry:
   * - `target_joint_id == j` (the joint this entry is for)
   * - `source_joint_id == affine_parent_[j]` (the joint's current root)
   * - `joint_value(j) = multiplier * joint_value(source_joint_id) + offset`
   * - For root joints (parent == self), the entry is the identity `(m=1, o=0)`.
   *
   * `add_affine_transmission` composes new edges into this flat form eagerly at the time of the
   * call, walking the affected group's member list to update every member's stored relation. The
   * planner then gets the (m, o) between any two joints in a group as an O(1) operation rather
   * than having to walk a chain.
   */
  struct AffineTransmission {
    /// The joint read by this affine transmission. By invariant, this is `target_joint_id`'s
    /// current affine root (= `affine_parent_[target_joint_id]`).
    JointId source_joint_id = 0;
    /// The joint written to by this affine transmission.
    JointId target_joint_id = 0;
    /// The composed multiplier from source to target. For non-root joints this is the product of
    /// every edge multiplier along the chain from the original source to the target. For root
    /// joints (entry's `source_joint_id == target_joint_id`) this is `1`.
    double multiplier = 1.0;
    /// The composed offset. For root joints this is `0`.
    double offset = 0.0;
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
  TransmissionAnalysis(TransmissionAnalysis &&) noexcept;
  TransmissionAnalysis & operator=(const TransmissionAnalysis & other);
  TransmissionAnalysis & operator=(TransmissionAnalysis &&) noexcept;
  ~TransmissionAnalysis();

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

  /**
   * Returns the current flat affine relationship of `j` to its affine group's root, in O(1).
   *
   * For root joints, returns the identity entry (m=1, o=0). For non-root joints, returns the
   * composed flat (m, o) such that `joint_value(j) = m * joint_value(root) + o`. Use
   * `affine_root_of(j)` to get the root joint id, or read it directly from the returned entry's
   * `source_joint_id` field.
   *
   * \pre `j` is a valid `JointId` previously returned by `ensure_joint_id` (debug-asserted).
   */
  [[nodiscard]] const AffineTransmission & affine_transmission_of(JointId j) const noexcept;

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

  /// Canonical mapping from symbolic interface ids to compact analysis-local kind ids.
  [[nodiscard]] const Order<InterfaceId, InterfaceKindId> & interface_order() const noexcept
  {
    return interface_order_;
  }

  /// Canonical mapping from symbolic interface ids to compact projection-rule ids.
  [[nodiscard]] const Order<InterfaceId, ProjectionKindId> & projection_order() const noexcept
  {
    return projection_order_;
  }

  /// Canonical mapping from `(JointId, InterfaceKindId)` to stable internal `StateInterfaceId`s.
  [[nodiscard]] const Order<CanonicalStateInterfaceDefinition, StateInterfaceId> &
  canonical_state_interface_order() const noexcept
  {
    return canonical_state_interface_order_;
  }

  /// Provides the JointId from joint_order_, adding it to the end of the order if it is not already present.
  JointId ensure_joint_id(const std::string & name);

  /**
   * Returns the `StateInterfaceId` for `definition` if it has been registered, or `std::nullopt`
   * if no `StateInterfaceId` has been assigned to this definition. Unlike `ensure_state_interface_id`,
   * this never mutates the analysis — it is safe to call from any const context.
   */
  [[nodiscard]] std::optional<StateInterfaceId>
  find_state_interface_id(const StateInterfaceDefinition & definition) const noexcept;

  [[nodiscard]] std::optional<StateInterfaceId>
  find_state_interface_id(const CanonicalStateInterfaceDefinition & definition) const noexcept;

  [[nodiscard]] std::optional<InterfaceKindId>
  find_interface_kind_id(const InterfaceId & interface_id) const noexcept;

  [[nodiscard]] const CanonicalStateInterfaceDefinition &
  canonical_state_interface_definition(StateInterfaceId state_interface_id) const noexcept
  {
    return canonical_state_interface_order_.inverse[state_interface_id];
  }

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
    double multiplier = 1.0,
    double offset = 0.0);

  /// Convenience overload — resolves joint names first.
  void add_affine_transmission(
    const std::string & source_joint_name,
    const std::string & target_joint_name,
    double multiplier = 1.0,
    double offset = 0.0);

  // ---------------------------------------------------------------------------
  // Affine projection rule registry
  // ---------------------------------------------------------------------------

  /// Sets (or replaces) the projection rule for a given interface id. After this, affine
  /// transmissions will project onto the specified interface using the given rule.
  void set_affine_projection_rule(InterfaceId interface_id, AffineProjectionRule rule);

  /// Returns a pointer to the rule for the given interface id, or `nullptr` if no rule is registered.
  /// An interface with no registered rule does not participate in affine projection at all.
  [[nodiscard]] const AffineProjectionRule * affine_projection_rule(const InterfaceId & interface_id) const noexcept;

  /// Returns the rule for a compact projection kind id.
  [[nodiscard]] const AffineProjectionRule & affine_projection_rule(ProjectionKindId projection_kind_id) const noexcept
  {
    return projection_rules_[projection_kind_id];
  }

  /// Returns the full registered projection-rule table. Useful for callers that need to enumerate
  /// every interface that participates in affine projection (e.g. ros2_control transmission import,
  /// which probes its plugin against this set instead of a hardcoded list of interface names).
  [[nodiscard]] const std::vector<AffineProjectionRule> & affine_projection_rules() const noexcept
  {
    return projection_rules_;
  }

  // ---------------------------------------------------------------------------
  // Affine group queries
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
  /// Indexed by `JointId` (one entry per joint). See `AffineTransmission`'s doc comment for the
  /// per-entry invariant. Maintained eagerly by `ensure_joint_id` (which appends an identity entry
  /// for the new joint) and `add_affine_transmission` (which composes the new edge into the
  /// existing flat relations and walks the affected group's member list to update them all).
  std::vector<AffineTransmission> affine_transmissions_{};
  std::vector<TransmissionInstance> transmissions_{};

  /// Joint name → JointId
  Order<std::string, JointId> joint_order_{};
  /// InterfaceId -> compact analysis-local kind id.
  Order<InterfaceId, InterfaceKindId> interface_order_{};
  /// InterfaceId -> compact projection-rule id.
  Order<InterfaceId, ProjectionKindId> projection_order_{};
  /// (JointId, InterfaceId) → StateInterfaceId
  Order<StateInterfaceDefinition, StateInterfaceId> state_interface_order_{};
  /// (JointId, InterfaceKindId) -> StateInterfaceId
  Order<CanonicalStateInterfaceDefinition, StateInterfaceId> canonical_state_interface_order_{};

  /// ProjectionKindId → projection rule. Populated with sane defaults for position, velocity,
  /// and acceleration in the constructor. Effort is *not* registered by default.
  std::vector<AffineProjectionRule> projection_rules_{};

  /// Inverse transmission index: state_interface_id → list of TransmissionInstanceIds whose
  /// `output_ids` contain that state interface. Indexed by StateInterfaceId (dense).
  ///
  /// **Invariant:** `producers_index_.size() == state_interface_order_.inverse.size()`.
  /// Maintained atomically by `ensure_state_interface_id` (the only entry point that grows the
  /// state interface set). `add_transmission` then appends to the relevant per-output entries.
  std::vector<std::vector<TransmissionInstanceId>> producers_index_{};

  /// Affine group index, indexed by JointId.
  ///
  /// **Invariants:**
  /// - `affine_parent_.size() == affine_group_members_storage_.size() == joint_order_.inverse.size()`.
  ///   Maintained atomically by `ensure_joint_id` (the only entry point that grows the joint set).
  /// - `affine_parent_[j]` is **always the direct root of `j`'s affine group**. The tree is kept
  ///   maximally flat: every union eagerly walks the loser group's member list and re-points each
  ///   member's parent to the new root. There is no lazy path compression on read — finds are O(1)
  ///   single array lookups.
  /// - The "root" of a group has a directional meaning: it is the source-side joint at the end of
  ///   the affine chain, the joint everything else ultimately derives from. Every
  ///   `add_affine_transmission(source, target, ...)` call makes `source`'s current root win the
  ///   merge against `target`'s current root.
  /// - `affine_group_members_storage_[root]` is the materialized member list for that root; for
  ///   non-root joints the entry is empty (members are migrated into the winning root on union).
  std::vector<JointId> affine_parent_{};
  std::vector<std::vector<JointId>> affine_group_members_storage_{};
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_ANALYSIS_HPP
