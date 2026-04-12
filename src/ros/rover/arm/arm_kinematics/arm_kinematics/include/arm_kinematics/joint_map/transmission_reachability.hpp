//
// Created by Bailey Chessum on 8/4/26.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_REACHABILITY_HPP
#define ARM_KINEMATICS_TRANSMISSION_REACHABILITY_HPP

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "arm_kinematics/joint_map/state_interface_producer.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/span.hpp"

namespace arm_kinematics {

/**
 * Build-time analysis of "what is derivable from these inputs?" against a `TransmissionAnalysis`.
 *
 * `TransmissionReachability` is the result of running a forward fixed point that propagates
 * derivability through transmission instances and affine projections. It is purely a function of
 * `(analysis, inputs)` — it does **not** know which outputs the caller wants. That concern lives
 * in `diagnose_missing_outputs` and `plan_joint_map`, which take a reachability plus an output
 * list.
 *
 * Construct via the static `analyze()` factory. The result is immutable: every query is a pure
 * read, safe for concurrent reads from multiple threads.
 *
 * **Interface classification.** Every state interface known to the analysis falls into exactly
 * one of four categories:
 *   - **Derivable** — has a unique committed producer. `producer_of(i)` returns a non-monostate
 *     value; `derivable_interfaces()` lists every such id.
 *   - **Ambiguous** — has ≥2 viable producers and the algorithm refuses to silently pick a winner.
 *     `producer_of(i)` returns `monostate`; `ambiguities()` lists the conflict.
 *   - **Transitively blocked** — has at least one potential producer transmission whose inputs
 *     are all classified (derivable, ambiguous, or transitively blocked) and at least one of
 *     those inputs is ambiguous or transitively blocked. The interface could be derivable if
 *     the user disambiguated upstream. `producer_of(i)` returns `monostate`;
 *     `transitively_blocked_interfaces()` lists every such id.
 *   - **Unproducible** — none of the above. The user simply hasn't supplied enough inputs and
 *     no producer chain exists that could possibly fire.
 *
 * Ambiguity does NOT propagate derivability: a transmission whose inputs include an ambiguous or
 * transitively blocked interface is silently skipped during the fixed point, so its outputs do
 * not enter the derivable set. This is correct: the runtime literally could not compute those
 * values.
 */
class TransmissionReachability {
public:
  // Reachability-local state interface ids. These are currently stored as a compact integer
  // type, but they belong to the reachability layer rather than to TransmissionAnalysis.
  using StateInterfaceId = std::size_t;

  /// One ambiguous interface in the reachability, with the full list of competing producers
  /// that the algorithm refused to silently choose between.
  ///
  /// Accumulated across the entire algorithm pass — `ambiguities()` returns *every* ambiguous
  /// interface, not just the first one encountered.
  ///
  /// Defined as a standalone type in `state_interface_producer.hpp`; aliased here so that
  /// existing code referring to `TransmissionReachability::AmbiguousInterface` continues to work.
  using AmbiguousInterface = producers::AmbiguousInterface;
  using StateInterfaceProducer = producers::StateInterfaceProducer;

  /// Pure-function analysis. Runs the eager forward fixed point against `analysis` starting from
  /// the given `inputs` (as definitions). Bare definitions — those with no registered
  /// `StateInterfaceId` in `analysis` — participate fully in affine propagation: any joint in an
  /// affine group can serve as a source or target regardless of SID registration.
  ///
  /// \warning **Lifetime contract:** the returned `TransmissionReachability` holds a reference
  /// to `analysis`. The reachability must not outlive the analysis. The type is intentionally
  /// non-movable and non-copyable so it can only be used as a local within the scope that owns
  /// (or otherwise guarantees the lifetime of) the analysis. Constructed via guaranteed copy
  /// elision — the prvalue returned here is materialized directly into the caller's storage.
  [[nodiscard]] static TransmissionReachability analyze(
    const TransmissionAnalysis & analysis,
    span<const StateInterfaceDefinition> inputs);

  ~TransmissionReachability() = default;

  // Non-copyable, non-movable — see the lifetime contract on `analyze()`. The reference to the
  // analysis is captured in the constructor and cannot be rebound.
  TransmissionReachability(const TransmissionReachability &) = delete;
  TransmissionReachability & operator=(const TransmissionReachability &) = delete;
  TransmissionReachability(TransmissionReachability &&) = delete;
  TransmissionReachability & operator=(TransmissionReachability &&) = delete;

  // ---------------------------------------------------------------------------
  // Queries
  // ---------------------------------------------------------------------------

  /// The underlying `TransmissionAnalysis` this reachability was analyzed against.
  [[nodiscard]] const TransmissionAnalysis & analysis() const noexcept { return analysis_; }

  /// True iff at least one interface in the reachability has multiple viable producers. The
  /// algorithm refuses to pick a winner and surfaces the conflict via `ambiguities()`.
  [[nodiscard]] bool is_ambiguous() const noexcept;

  /// The original `inputs` span passed to `analyze()`, in insertion order. Duplicate entries are
  /// preserved (the same interface can appear multiple times); positions in this span correspond
  /// to `producers::Input::input_index` for the **first occurrence** of each interface.
  [[nodiscard]] span<const StateInterfaceDefinition> inputs() const noexcept;

  /// O(1) lookup for any definition — registered or bare. For definitions that have a real
  /// `StateInterfaceId`, delegates to `producer_of(sid)`. For bare definitions (no registered
  /// SID), checks the internal `def_producer_assignment_` populated during affine propagation.
  /// Returns `std::monostate` if the definition is not derivable.
  [[nodiscard]] StateInterfaceProducer producer_of_def(
    const StateInterfaceDefinition & def) const noexcept;

  /// The full set of derivable interfaces — user inputs plus everything reachable through
  /// transmissions and affine projections, in algorithm-discovery order. Use this to ask "is X
  /// part of the working set?" rather than "did the user supply X?".
  [[nodiscard]] span<const StateInterfaceId> derivable_interfaces() const noexcept;

  /// Every ambiguous interface with its competing candidate producers. Accumulated across the
  /// full algorithm pass, not just the first conflict.
  [[nodiscard]] span<const AmbiguousInterface> ambiguities() const noexcept;

  /// Interfaces that cannot be computed because at least one of their potential producers'
  /// inputs is itself ambiguous or transitively blocked. Disjoint from `derivable_interfaces()`
  /// and from `ambiguities()`. The user resolves these by disambiguating the relevant upstream
  /// — see `diagnose_missing_outputs` for the per-output attribution walk.
  [[nodiscard]] span<const StateInterfaceId> transitively_blocked_interfaces() const noexcept;

  /// Interfaces in the analysis that are NOT in any of `derivable_interfaces()`,
  /// `ambiguities()`, or `transitively_blocked_interfaces()` — i.e., they have no potential
  /// producer that could fire from the current inputs (the user simply hasn't supplied enough).
  /// The four sets together partition the analysis's state interfaces.
  [[nodiscard]] span<const StateInterfaceId> unproducible_interfaces() const noexcept;

  /// O(1) lookup. Returns the committed producer for `interface`, or `std::monostate` if the
  /// interface is ambiguous, transitively blocked, unproducible, or not known to the analysis.
  /// The four non-derivable cases are distinguished by `ambiguities()`,
  /// `transitively_blocked_interfaces()`, and the
  /// state interface order respectively (or via `diagnose_missing_outputs` for a one-shot
  /// classification of a request).
  [[nodiscard]] StateInterfaceProducer producer_of(StateInterfaceId interface) const noexcept;

  /// Each entry: a leaf input that the algorithm did NOT pick as the affine projection source
  /// for its (joint, interface) affine group, because a lower-`JointId` leaf in the same group
  /// already won. Surfaces the case where the user supplied two mathematically-equivalent
  /// inputs in the same affine group — both are still stored as `Input` producers for
  /// themselves, but only one is used to project to the other group members.
  [[nodiscard]] span<const StateInterfaceDefinition> redundant_equivalent_inputs() const noexcept;

private:
  /// Private constructor used by `analyze()`. Captures the analysis reference and immediately
  /// runs the fixed point. Direct construction in the prvalue returned by `analyze()` is the
  /// only path to a `TransmissionReachability`.
  explicit TransmissionReachability(
    const TransmissionAnalysis & analysis,
    span<const StateInterfaceDefinition> inputs);

  // Helper: derives the interface-space (m, o) from the source joint's interface to the target
  // joint's interface using the per-joint flat relations stored on the analysis. The interface
  // id is **implicit** — encoded by the choice of `rule`. Joint-level math is interface-agnostic
  // and only depends on the per-joint flats stored on the analysis; the rule then transforms the
  // joint-level result into interface-space (multiplier_scale, offset_scale, reverse_direction).
  //
  // \pre Both joints must be in the same affine group (same `affine_root_of`).
  // \pre All multipliers in the chain must be non-zero — guaranteed by `add_affine_transmission`.
  struct AffineProjectionCoefficients {
    double multiplier = 1.0;
    double offset = 0.0;
  };
  AffineProjectionCoefficients compute_affine_projection_coefficients(
    JointId source_joint,
    JointId target_joint,
    const AffineProjectionRule & rule) const noexcept;

  void run_fixed_point(span<const StateInterfaceDefinition> inputs);

  /// True iff `def` is currently in the derivable set — checks both the SID-indexed
  /// `derivable_membership_` (for registered defs) and `derivable_bare_defs_set_` (for bare defs).
  [[nodiscard]] bool is_derivable_def(const StateInterfaceDefinition & def) const noexcept;

  /// Like `add_to_derivable` but for bare defs (no registered SID). Inserts into
  /// `derivable_bare_defs_set_` and `derivable_bare_defs_list_`. Returns true iff newly added.
  bool add_to_derivable_bare_def(const StateInterfaceDefinition & def);

  // ---- Algorithm helpers (called from `run_fixed_point`) ---------------------

  /// Returns true iff `sid` was newly added to `derivable_membership_`. No-op for sids that
  /// are already derivable or out of range.
  bool add_to_derivable(StateInterfaceId sid);

  /// Records `producer` as a candidate for `iface`, honoring input-wins (if `iface` already has
  /// an `Input` entry in `producer_assignment_`, the candidate is silently dropped).
  void record_candidate(
    StateInterfaceId iface,
    StateInterfaceProducer producer,
    std::vector<std::vector<StateInterfaceProducer>> & candidates) const;

  /// Process a single `(group, interface_id)` affine hyper-node: pick the lowest-JointId leaf
  /// source among the group's currently-derivable members and project from it to every other
  /// group member, recording AffineProjection candidates and (when not already known
  /// ambiguous) marking them derivable. Sets `*changed = true` if any new interface entered
  /// `derivable_membership_`.
  ///
  /// The leaf-source picking is dynamic — the union-find affine root is irrelevant; whichever
  /// derivable group member happens to be a leaf (Input or unique Transmission candidate)
  /// wins, with ties broken by lowest JointId. This is what enables the "user can supply any
  /// group member as input" pivoting property.
  void process_affine_hypernode(
    const InterfaceId & interface_id,
    const AffineProjectionRule & rule,
    span<const JointId> group_members,
    std::vector<std::vector<StateInterfaceProducer>> & candidates,
    bool & changed);

  /// Run the transitively-blocked post-pass after the main 2-pass algorithm converges. Walks
  /// transmissions and affine groups, marking interfaces as transitively blocked iff they have
  /// at least one potential producer whose inputs are all classified (derivable, ambiguous, or
  /// transitively blocked) and at least one is ambiguous or transitively blocked. Iterates to
  /// a fixed point — only runs when at least one ambiguity exists.
  void run_transitively_blocked_post_pass(std::size_t state_count);

  // ---------------------------------------------------------------------------
  // State
  // ---------------------------------------------------------------------------

  /// Captured at construction. The reachability is non-movable so this reference can't be
  /// rebound. The lifetime contract on `analyze()` requires the analysis to outlive `*this`.
  const TransmissionAnalysis & analysis_;

  /// Effective input list — `analyze()`'s inputs argument, in insertion order. Duplicates are
  /// preserved; producer assignment uses first-occurrence-wins semantics.
  std::vector<StateInterfaceDefinition> inputs_{};

  /// Derivable interfaces in algorithm-discovery order.
  std::vector<StateInterfaceId> derivable_interfaces_{};

  /// Membership lookup for derivable_interfaces_, indexed by StateInterfaceId. Sized to match
  /// the analysis's state interface count at fixed-point time.
  std::vector<bool> derivable_membership_{};

  /// Producer assignment for each derivable interface that has a unique producer.
  std::vector<std::optional<StateInterfaceProducer>> producer_assignment_{};

  /// Interfaces with multiple viable producers, with their full candidate lists.
  std::vector<AmbiguousInterface> ambiguities_{};

  /// Membership lookup for ambiguous interfaces, indexed by StateInterfaceId. Used by the inner
  /// fixed point's "skip transmissions whose inputs are ambiguous" check, and by the
  /// transitively-blocked post-pass.
  std::vector<bool> ambiguous_membership_{};

  /// Interfaces that have at least one potential producer transmission whose inputs include
  /// (transitively) an ambiguous interface, but that themselves have no derivable producer.
  /// Computed in a post-pass after the main fixed point converges; only populated when at least
  /// one ambiguity exists.
  std::vector<StateInterfaceId> transitively_blocked_interfaces_{};

  /// Membership lookup for transitively_blocked_interfaces_, indexed by StateInterfaceId.
  std::vector<bool> transitively_blocked_membership_{};

  /// Interfaces in the analysis that fall into none of derivable / ambiguous /
  /// transitively_blocked. Computed eagerly at the end of `run_fixed_point` (cheap O(N) walk).
  /// The numeric order is not algorithmically meaningful — the field exists for ergonomic
  /// completeness so consumers can iterate "everything still missing".
  std::vector<StateInterfaceId> unproducible_interfaces_{};

  /// Redundant equivalent inputs (lower-JointId leaf wins in affine groups). Stored as
  /// definitions rather than SIDs because bare inputs (no SID) can also be redundant.
  std::vector<StateInterfaceDefinition> redundant_equivalent_inputs_{};

  // ---------------------------------------------------------------------------
  // Bare-def tracking (affine projections to/from definitions with no registered SID)
  // ---------------------------------------------------------------------------

  /// Producer assignment for bare definitions (those with no registered `StateInterfaceId`).
  /// Populated by `process_affine_hypernode` when projecting to group members that lack a SID.
  /// Bare defs can only have `Input` or `AffineProjection` producers (never `Transmission`).
  std::unordered_map<StateInterfaceDefinition, StateInterfaceProducer> def_producer_assignment_{};

  /// Membership set for `def_producer_assignment_` — O(1) derivability check for bare defs.
  std::unordered_set<StateInterfaceDefinition> derivable_bare_defs_set_{};

  /// Bare derivable defs in discovery order, for inclusion in the affine hypernode snapshot loop.
  std::vector<StateInterfaceDefinition> derivable_bare_defs_list_{};
};

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_TRANSMISSION_REACHABILITY_HPP
