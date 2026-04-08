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
 * `TransmissionReachability` is the result of running an eager forward fixed point that
 * propagates derivability through transmission instances and affine projections. It is purely
 * a function of `(analysis, inputs)` — it does **not** know which outputs the caller wants. That
 * concern lives in `diagnose_missing_outputs` and `plan_joint_map`, which take a reachability
 * plus an output list.
 *
 * Construct via the static `analyze()` factory. The result is immutable: every query is a pure
 * read, safe for concurrent reads from multiple threads.
 *
 * \note **Ambiguity-poison limitation.** When an interface is ambiguous (≥2 viable producers),
 * the algorithm still propagates its derivability to downstream transmissions — a transmission
 * whose input is ambiguous will still record its outputs as candidates and may end up with a
 * committed `producer_of()` entry. Those downstream commits are **internally inconsistent**:
 * their inputs cannot actually be computed at runtime because the upstream is ambiguous. This is
 * a deliberate simplicity trade-off — propagating ambiguity poison correctly would require
 * multiple fixed-point passes. Consumers (`diagnose_missing_outputs`, `plan_joint_map`) refuse to
 * proceed when `is_ambiguous()` is true and surface the upstream `Ambiguous` error first, so the
 * inconsistency never reaches a runtime `JointMap`.
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

  /// O(1) lookup. Returns `std::monostate` if the interface is not derivable from the analyzed
  /// inputs, ambiguous, or out-of-scope (not known to the analysis).
  ///
  /// \warning When `is_ambiguous()` is true, this method may return non-`monostate` values for
  /// interfaces whose producers transitively depend on ambiguous upstream interfaces. Those
  /// values are internally inconsistent and must not be used to emit a runtime `JointMap`. The
  /// downstream consumers (`diagnose_missing_outputs`, `plan_joint_map`) gate on
  /// `is_ambiguous()` and refuse to proceed in that case. See the class-level
  /// "Ambiguity-poison limitation" note for the rationale.
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

  /// Redundant equivalent inputs (lower-JointId leaf wins in affine groups).
  std::vector<StateInterfaceId> redundant_equivalent_inputs_{};
};

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_TRANSMISSION_REACHABILITY_HPP
