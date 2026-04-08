//
// Created by Bailey Chessum on 5/4/26.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_SUBGRAPH_HPP
#define ARM_KINEMATICS_TRANSMISSION_SUBGRAPH_HPP

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

/// The interface's value comes directly from one of the request's input slots.
struct Input {
  /// Position in the subgraph's effective inputs span (i.e. the index a builder uses to read
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

/// What produces a given `StateInterfaceId` in the current plan?
///
/// `std::monostate` represents "not produced" — the interface is unreachable, ambiguous (in
/// which case the producer is intentionally hidden because the algorithm refuses to pick a
/// winner), or out-of-scope (not in the request's inputs/outputs and not transitively
/// reachable). Builders should check `TransmissionSubgraph::is_ambiguous()` before walking
/// outputs.
using StateInterfaceProducer = std::variant<
  std::monostate,
  producers::Input,
  producers::AffineProjection,
  producers::Transmission
>;

/**
 * Build-time analysis utility that captures the relevant subgraph of a `TransmissionAnalysis`
 * for a given `(inputs, outputs)` request, supports incremental modification, and answers
 * planning queries.
 *
 * The algorithm runs **eagerly** in the constructor and at the end of every mutation, so
 * post-construction (and post-mutation) every query is a pure read with the strong invariants
 * documented inline.
 *
 * \note Read-only queries are pure reads — no internal mutation. Safe for concurrent reads
 * from multiple threads as long as no thread is mutating.
 */
class TransmissionSubgraph {
public:
  /// One ambiguous interface in the subgraph's plan, with the full list of competing producers
  /// that the algorithm refused to silently choose between.
  ///
  /// Accumulated across the entire algorithm pass — `ambiguous_interfaces()` returns *every*
  /// ambiguous interface, not just the first one encountered. Builders surface the full set so
  /// the user can fix all conflicts in one round-trip.
  struct AmbiguousInterface {
    StateInterfaceId interface = 0;
    std::vector<StateInterfaceProducer> candidates{};
  };

  /// One interface that was added via `add_input()` and which had pre-existing non-Input
  /// candidate producers that the new Input override discarded. Builders can surface these as
  /// warnings ("you supplied X explicitly, but the analysis says X is also derivable via
  /// transmission Y — using your value, here's what we discarded").
  ///
  /// Note: a discarded `Transmission` candidate in this list does NOT mean the transmission is
  /// unselected entirely. It only means the transmission is no longer the chosen producer FOR
  /// THIS interface. The same transmission may still appear in `selected_transmissions()`
  /// because it produces other needed outputs.
  ///
  /// Only populated when `add_input()` actually overrode something — empty for `initial_inputs`
  /// (which are exclusive without warning, since they were declared at construction time).
  struct InputOverride {
    StateInterfaceId interface = 0;
    std::vector<StateInterfaceProducer> discarded_candidates{};
  };

  TransmissionSubgraph(
    const TransmissionAnalysis & analysis,
    span<const StateInterfaceId> initial_inputs,
    span<const StateInterfaceId> initial_outputs);

  ~TransmissionSubgraph() = default;

  // Move-only — copying a subgraph would be expensive and there's no clear use case.
  TransmissionSubgraph(const TransmissionSubgraph &) = delete;
  TransmissionSubgraph & operator=(const TransmissionSubgraph &) = delete;
  TransmissionSubgraph(TransmissionSubgraph &&) noexcept = default;
  TransmissionSubgraph & operator=(TransmissionSubgraph &&) noexcept = default;

  // ---------------------------------------------------------------------------
  // Mutation
  // ---------------------------------------------------------------------------

  /// Append `interface` to the effective input list and re-run the fixed point.
  ///
  /// First-occurrence wins: if `interface` is not yet in the effective input list, it gets a
  /// fresh `producers::Input` and any pre-existing non-Input candidate producers for it are
  /// discarded into `input_overrides()`. If it's already in the list (whether from
  /// `initial_inputs` or a previous `add_input` call), this just records the duplicate slot in
  /// `requested_inputs()` — `producer_of()` keeps pointing at the first occurrence and no
  /// override entry is added. Spans returned by previous query calls are invalidated.
  void add_input(StateInterfaceId interface);

  /// Append `interface` to the needed outputs list and re-run classification.
  ///
  /// Spans returned by previous query calls are invalidated.
  void add_output(StateInterfaceId interface);

  // ---------------------------------------------------------------------------
  // Queries
  // ---------------------------------------------------------------------------

  /// The underlying `TransmissionAnalysis` this subgraph was built from. Step 6's
  /// `compute_missing_input_resolutions` reaches in here to walk the producing-transmissions
  /// inverse index and the affine group index.
  [[nodiscard]] const TransmissionAnalysis & analysis() const noexcept { return analysis_; }

  /// True iff every needed output is produced by exactly one viable producer (i.e. no
  /// unreachable outputs and no ambiguous interfaces).
  [[nodiscard]] bool is_complete() const noexcept;

  /// True iff at least one interface in the plan has multiple viable producers. The algorithm
  /// refuses to pick a winner and surfaces the conflict via `ambiguous_interfaces()`.
  [[nodiscard]] bool is_ambiguous() const noexcept;

  /// The original `initial_inputs` span passed to the constructor, plus any interfaces appended
  /// via `add_input()`, in insertion order. Duplicate entries are preserved (the same interface
  /// can appear multiple times in the list); positions in this span correspond to
  /// `producers::Input::input_index` for the **first occurrence** of each interface.
  [[nodiscard]] span<const StateInterfaceId> requested_inputs() const noexcept;

  /// The full set of derivable interfaces — user inputs plus everything reachable through
  /// transmissions and affine projections, in algorithm-discovery order. Use this to ask "is X
  /// part of the working set?" rather than "did the user supply X?".
  [[nodiscard]] span<const StateInterfaceId> derivable_interfaces() const noexcept;

  /// Needed outputs that the algorithm could not derive from the requested inputs. Empty when
  /// the plan is complete.
  [[nodiscard]] span<const StateInterfaceId> unreachable_outputs() const noexcept;

  /// Every ambiguous interface with its competing candidate producers. Accumulated across the
  /// full algorithm pass, not just the first conflict.
  [[nodiscard]] span<const AmbiguousInterface> ambiguous_interfaces() const noexcept;

  /// Selected transmission instances in topological order (dependencies before dependents).
  /// Builders walk this list to emit transmission stages in execution order.
  [[nodiscard]] span<const TransmissionInstanceId> selected_transmissions() const noexcept;

  /// True iff `interface` has a unique non-ambiguous producer in the plan (i.e.
  /// `producer_of(interface)` would return something other than `monostate`).
  [[nodiscard]] bool produces(StateInterfaceId interface) const noexcept;

  /// The principal builder query: how is this state interface produced in the plan?
  ///
  /// O(1) lookup. Returns `std::monostate` if the interface is unreachable, ambiguous, or
  /// out-of-scope (not in `derivable_interfaces()`). Builders should check `is_ambiguous()`
  /// before walking outputs.
  [[nodiscard]] StateInterfaceProducer producer_of(StateInterfaceId interface) const noexcept;

  /// Each entry: an interface that was added via `add_input()` and which had pre-existing
  /// non-Input candidate producers that the new Input override discarded. Empty for
  /// `initial_inputs` (which are exclusive without warning).
  [[nodiscard]] span<const InputOverride> input_overrides() const noexcept;

  /// Each entry: a leaf input that the algorithm did NOT pick as the affine projection source
  /// for its (joint, interface) affine group, because a lower-`JointId` leaf in the same group
  /// already won. Surfaces the case where the user supplied two mathematically-equivalent
  /// inputs in the same affine group — both are still stored as `Input` producers for
  /// themselves, but only one is used to project to the other group members.
  [[nodiscard]] span<const StateInterfaceId> redundant_equivalent_inputs() const noexcept;

private:
  // Eager fixed-point algorithm. Called by the constructor and by every mutation.
  void run_fixed_point();

  // Helper: collect every TransmissionInstance into selected_transmissions_ that's reachable
  // (transitively) from a needed output's producer chain, then topologically sort the result.
  void emit_selected_transmissions();

  // Helper: derives the interface-space (m, o) from the source joint's interface to the target
  // joint's interface using the per-joint flat relations stored on the analysis. Both joints
  // must be in the same affine group, the interface id must have a registered projection rule,
  // and the multipliers must be non-zero (guaranteed by add_affine_transmission).
  // Result is in interface-space, after applying the projection rule's transformations.
  struct AffineProjectionCoefficients {
    float multiplier = 1.0F;
    float offset = 0.0F;
  };
  AffineProjectionCoefficients compute_affine_projection_coefficients(
    JointId source_joint,
    JointId target_joint,
    const AffineProjectionRule & rule) const noexcept;

  // ---------------------------------------------------------------------------
  // State
  // ---------------------------------------------------------------------------

  const TransmissionAnalysis & analysis_;

  /// Effective input list — initial_inputs plus add_input additions, in insertion order.
  /// Duplicates are preserved; producer assignment uses first-occurrence-wins semantics.
  std::vector<StateInterfaceId> requested_inputs_{};

  /// Needed outputs (initial_outputs plus add_output additions, in insertion order). Not
  /// deduplicated against requested_inputs — interfaces in both are handled in run_fixed_point.
  std::vector<StateInterfaceId> needed_outputs_{};

  /// Derivable interfaces in algorithm-discovery order. Recomputed by run_fixed_point.
  std::vector<StateInterfaceId> derivable_interfaces_{};

  /// Membership lookup for derivable_interfaces_, indexed by StateInterfaceId. Sized to match
  /// the analysis's state interface count at fixed-point time.
  std::vector<bool> derivable_membership_{};

  /// Producer assignment for each derivable interface that has a unique producer.
  std::unordered_map<StateInterfaceId, StateInterfaceProducer> producer_assignment_{};

  /// Needed outputs that have no viable producer.
  std::vector<StateInterfaceId> unreachable_outputs_{};

  /// Interfaces with multiple viable producers, with their full candidate lists.
  std::vector<AmbiguousInterface> ambiguous_interfaces_{};

  /// Membership lookup for ambiguous interfaces, indexed by StateInterfaceId. Used to quickly
  /// answer "is this in the ambiguous set?" during needed-output classification.
  std::vector<bool> ambiguous_membership_{};

  /// Topologically-sorted transmission instances that are needed for the plan.
  std::vector<TransmissionInstanceId> selected_transmissions_{};

  /// Sticky log of input_overrides events, accumulated across mutation calls. Cleared only
  /// when the constructor runs (i.e. survives `add_input` / `add_output` calls).
  std::vector<InputOverride> input_overrides_{};

  /// Sticky log of redundant equivalent inputs, recomputed each fixed-point pass.
  std::vector<StateInterfaceId> redundant_equivalent_inputs_{};
};

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_TRANSMISSION_SUBGRAPH_HPP
