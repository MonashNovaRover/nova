//
// Created by Bailey Chessum on 8/4/26.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_REACHABILITY_HPP
#define ARM_KINEMATICS_TRANSMISSION_REACHABILITY_HPP

#include <cstddef>
#include <unordered_map>
#include <variant>
#include <vector>

#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/span.hpp"

namespace arm_kinematics {

/**
 * Producer alternatives carried in `StateInterfaceProducer`.
 *
 * Each alternative carries only the data relevant to its case (no fat-struct anti-pattern with
 * always-empty fields). The variant `StateInterfaceProducer` selects between them.
 */
namespace producers {

/// The interface's value comes directly from one of the analysis-input slots.
struct Input {
  /// Position in the reachability's effective inputs span (i.e. the index a builder uses to read
  /// from the JointMap's `inputs` array at runtime). Always points at the first occurrence of
  /// the interface in the input list — duplicates of the same interface do not change this
  /// index.
  std::size_t input_index = 0;
};

/// The interface's value is derived from another known leaf interface via a (possibly composed)
/// affine relationship. The `source` is **always** another producer that resolves to either
/// `Input` or `Transmission` in a single `producer_of()` step — never another `AffineProjection`.
/// The algorithm enforces this by preferring leaf interfaces when picking a source.
struct AffineProjection {
  /// The known leaf interface providing the underlying value.
  StateInterfaceId source = 0;
  /// Already-composed interface-space coefficient. Computed in O(1) at planning time from the
  /// per-joint flat affine relations stored in `TransmissionAnalysis::affine_transmissions_`
  /// and the projection rule for the relevant interface id.
  float multiplier = 1.0F;
  float offset = 0.0F;
};

/// The interface's value is produced by a selected `TransmissionInstance` (a block compute
/// that may produce multiple values together).
struct Transmission {
  TransmissionInstanceId instance_id = 0;
};

}  // namespace producers

/// What produces a given `StateInterfaceId` in the current reachability?
///
/// `std::monostate` represents "not produced" — the interface is unreachable from the analyzed
/// inputs, ambiguous (in which case the producer is intentionally hidden because the algorithm
/// refuses to pick a winner), or out-of-scope (not in the inputs and not transitively reachable).
using StateInterfaceProducer = std::variant<
  std::monostate,
  producers::Input,
  producers::AffineProjection,
  producers::Transmission
>;

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
 *   - **Blocked** — has at least one potential producer transmission whose inputs are all known
 *     OR ambiguous OR blocked, and at least one input is ambiguous or blocked. The interface
 *     could be derivable if the user disambiguated upstream. `producer_of(i)` returns
 *     `monostate`; `blocked_interfaces()` lists every such id.
 *   - **Unreachable** — none of the above. The user simply hasn't supplied enough inputs.
 *
 * Ambiguity does NOT propagate derivability: a transmission whose inputs include an ambiguous or
 * blocked interface is silently skipped during the fixed point, so its outputs do not enter the
 * derivable set. This is correct: the runtime literally could not compute those values.
 */
class TransmissionReachability {
public:
  /// One ambiguous interface in the reachability, with the full list of competing producers
  /// that the algorithm refused to silently choose between.
  ///
  /// Accumulated across the entire algorithm pass — `ambiguities()` returns *every* ambiguous
  /// interface, not just the first one encountered.
  struct AmbiguousInterface {
    StateInterfaceId interface = 0;
    std::vector<StateInterfaceProducer> candidates{};
  };

  /// Pure-function analysis. Runs the eager forward fixed point against `analysis` starting from
  /// the given `inputs`. The returned object is immutable — to update the analysis with more
  /// inputs, call `analyze()` again with the augmented input list.
  [[nodiscard]] static TransmissionReachability analyze(
    const TransmissionAnalysis & analysis,
    span<const StateInterfaceId> inputs);

  ~TransmissionReachability() = default;

  // Move-only — copying would be expensive and there's no clear use case.
  TransmissionReachability(const TransmissionReachability &) = delete;
  TransmissionReachability & operator=(const TransmissionReachability &) = delete;
  TransmissionReachability(TransmissionReachability &&) noexcept = default;
  TransmissionReachability & operator=(TransmissionReachability &&) noexcept = default;

  // ---------------------------------------------------------------------------
  // Queries
  // ---------------------------------------------------------------------------

  /// The underlying `TransmissionAnalysis` this reachability was analyzed against.
  [[nodiscard]] const TransmissionAnalysis & analysis() const noexcept { return *analysis_; }

  /// True iff at least one interface in the reachability has multiple viable producers. The
  /// algorithm refuses to pick a winner and surfaces the conflict via `ambiguities()`.
  [[nodiscard]] bool is_ambiguous() const noexcept;

  /// The original `inputs` span passed to `analyze()`, in insertion order. Duplicate entries are
  /// preserved (the same interface can appear multiple times); positions in this span correspond
  /// to `producers::Input::input_index` for the **first occurrence** of each interface.
  [[nodiscard]] span<const StateInterfaceId> inputs() const noexcept;

  /// The full set of derivable interfaces — user inputs plus everything reachable through
  /// transmissions and affine projections, in algorithm-discovery order. Use this to ask "is X
  /// part of the working set?" rather than "did the user supply X?".
  [[nodiscard]] span<const StateInterfaceId> derivable_interfaces() const noexcept;

  /// Every ambiguous interface with its competing candidate producers. Accumulated across the
  /// full algorithm pass, not just the first conflict.
  [[nodiscard]] span<const AmbiguousInterface> ambiguities() const noexcept;

  /// Interfaces that cannot be computed because at least one of their potential producers'
  /// inputs is itself ambiguous or blocked. Disjoint from `derivable_interfaces()` and from
  /// `ambiguities()`. The user resolves these by disambiguating the relevant upstream — see
  /// `diagnose_missing_outputs` for the per-output attribution walk.
  [[nodiscard]] span<const StateInterfaceId> blocked_interfaces() const noexcept;

  /// O(1) lookup. Returns the committed producer for `interface`, or `std::monostate` if the
  /// interface is ambiguous, blocked, unreachable, or not known to the analysis. The four
  /// non-derivable cases are distinguished by `ambiguities()`, `blocked_interfaces()`, and the
  /// state interface order respectively (or via `diagnose_missing_outputs` for a one-shot
  /// classification of a request).
  [[nodiscard]] StateInterfaceProducer producer_of(StateInterfaceId interface) const noexcept;

  /// Each entry: a leaf input that the algorithm did NOT pick as the affine projection source
  /// for its (joint, interface) affine group, because a lower-`JointId` leaf in the same group
  /// already won. Surfaces the case where the user supplied two mathematically-equivalent
  /// inputs in the same affine group — both are still stored as `Input` producers for
  /// themselves, but only one is used to project to the other group members.
  [[nodiscard]] span<const StateInterfaceId> redundant_equivalent_inputs() const noexcept;

private:
  TransmissionReachability() = default;

  // Helper: derives the interface-space (m, o) from the source joint's interface to the target
  // joint's interface using the per-joint flat relations stored on the analysis. The interface
  // id is **implicit** — encoded by the choice of `rule`. Joint-level math is interface-agnostic
  // and only depends on the per-joint flats stored on the analysis; the rule then transforms the
  // joint-level result into interface-space (multiplier_scale, offset_scale, reverse_direction).
  //
  // \pre Both joints must be in the same affine group (same `affine_root_of`).
  // \pre All multipliers in the chain must be non-zero — guaranteed by `add_affine_transmission`.
  struct AffineProjectionCoefficients {
    float multiplier = 1.0F;
    float offset = 0.0F;
  };
  AffineProjectionCoefficients compute_affine_projection_coefficients(
    JointId source_joint,
    JointId target_joint,
    const AffineProjectionRule & rule) const noexcept;

  void run_fixed_point(span<const StateInterfaceId> inputs);

  // ---- Algorithm helpers (called from `run_fixed_point`) ---------------------

  /// Returns true iff `sid` was newly added to `derivable_membership_`. No-op for sids that
  /// are already derivable or out of range.
  bool add_to_derivable(StateInterfaceId sid);

  /// Records `producer` as a candidate for `iface`, honoring input-wins (if `iface` already has
  /// an `Input` entry in `producer_assignment_`, the candidate is silently dropped).
  void record_candidate(
    StateInterfaceId iface,
    StateInterfaceProducer producer,
    std::unordered_map<StateInterfaceId, std::vector<StateInterfaceProducer>> & candidates) const;

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
    JointId root,
    const InterfaceId & interface_id,
    const AffineProjectionRule & rule,
    span<const JointId> group_members,
    std::unordered_map<StateInterfaceId, std::vector<StateInterfaceProducer>> & candidates,
    bool & changed);

  /// Run the blocked-interface post-pass after the main 2-pass algorithm converges. Walks
  /// transmissions and affine groups, marking interfaces as blocked iff they have at least one
  /// potential producer whose inputs are all classified (derivable, ambiguous, or blocked) and
  /// at least one is ambiguous or blocked. Iterates to a fixed point — only runs when at least
  /// one ambiguity exists.
  void run_blocked_post_pass(std::size_t state_count);

  // ---------------------------------------------------------------------------
  // State
  // ---------------------------------------------------------------------------

  /// Pointer (not reference) so the type stays move-assignable. Set by `analyze()`; never null
  /// post-construction.
  const TransmissionAnalysis * analysis_ = nullptr;

  /// Effective input list — `analyze()`'s inputs argument, in insertion order. Duplicates are
  /// preserved; producer assignment uses first-occurrence-wins semantics.
  std::vector<StateInterfaceId> inputs_{};

  /// Derivable interfaces in algorithm-discovery order.
  std::vector<StateInterfaceId> derivable_interfaces_{};

  /// Membership lookup for derivable_interfaces_, indexed by StateInterfaceId. Sized to match
  /// the analysis's state interface count at fixed-point time.
  std::vector<bool> derivable_membership_{};

  /// Producer assignment for each derivable interface that has a unique producer.
  std::unordered_map<StateInterfaceId, StateInterfaceProducer> producer_assignment_{};

  /// Interfaces with multiple viable producers, with their full candidate lists.
  std::vector<AmbiguousInterface> ambiguities_{};

  /// Membership lookup for ambiguous interfaces, indexed by StateInterfaceId. Used by the inner
  /// fixed point's "skip transmissions whose inputs are ambiguous" check, and by the blocked
  /// post-pass.
  std::vector<bool> ambiguous_membership_{};

  /// Interfaces that have at least one potential producer transmission whose inputs include
  /// (transitively) an ambiguous interface, but that themselves have no derivable producer.
  /// Computed in a post-pass after the main fixed point converges; only populated when at least
  /// one ambiguity exists.
  std::vector<StateInterfaceId> blocked_interfaces_{};

  /// Membership lookup for blocked_interfaces_, indexed by StateInterfaceId.
  std::vector<bool> blocked_membership_{};

  /// Redundant equivalent inputs (lower-JointId leaf wins in affine groups).
  std::vector<StateInterfaceId> redundant_equivalent_inputs_{};
};

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_TRANSMISSION_REACHABILITY_HPP
