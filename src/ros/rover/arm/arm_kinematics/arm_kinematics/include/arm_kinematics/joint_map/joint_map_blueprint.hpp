//
// Created by Bailey Chessum on 8/4/26.
//

#ifndef ARM_KINEMATICS_JOINT_MAP_BLUEPRINT_HPP
#define ARM_KINEMATICS_JOINT_MAP_BLUEPRINT_HPP

#include <cstddef>
#include <optional>
#include <variant>
#include <vector>

#include "arm_kinematics/joint_map/transmission_reachability.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/order.hpp"
#include "arm_kinematics/utilities/span.hpp"

namespace arm_kinematics {

// Blueprint-local state interface ids. These are currently stored as a compact integer type, but
// they are intentionally scoped to the blueprint layer rather than borrowed from
// TransmissionAnalysis.
using BlueprintStateInterfaceId = std::size_t;

struct BlueprintStateInterfaceRecord {
  StateInterfaceDefinition definition{};
  std::optional<TransmissionAnalysis::StateInterfaceId> analysis_state_interface_id{};
};

/**
 * One stage in a `JointMapBlueprint`.
 *
 * A blueprint is a sequence of these segments, executed in order. Each segment either runs a
 * transmission instance or computes a batch of affine-mapped outputs from values that are
 * already available (i.e., inputs and outputs of previously-executed transmission stages).
 *
 * At runtime, the materializer maintains a logical "value table" keyed by
 * `TransmissionAnalysis::StateInterfaceId` —
 * inputs are seeded into it before the first segment runs, and each transmission stage adds its
 * outputs to it. Affine batches read source values out of the table and write results into the
 * blueprint's overall output buffer.
 */
struct JointMapBlueprintSegment {
  /// A run of outputs whose producer is either `Input` or `AffineProjection` and whose source
  /// values are *already available* at this point in the segment sequence (i.e., they come from
  /// the input vector or from outputs of an earlier `TransmissionStage` in the same blueprint).
  ///
  /// All four parallel vectors have the same length. Each row `i` computes:
  /// \code
  ///   blueprint_output[blueprint_output_indices[i]] = multipliers[i] * value(sources[i]) + offsets[i]
  /// \endcode
  /// where `value(sid)` is read from the runtime value table.
  ///
  /// **Affine batching:** consecutive output positions whose producers can all be served from
  /// the values available at the same execution point are folded into a single `InputAffineBatch`
  /// segment. This is the consolidation rule called out in the refactor plan doc — for a request
  /// with N mimic outputs from one input, the blueprint produces one batch with N rows, not N
  /// separate segments.
  struct InputAffineBatch {
    /// Where to write each row's result, in the blueprint's overall ordered output buffer.
    std::vector<std::size_t> blueprint_output_indices{};
    /// Which blueprint-local state interface to read for each row. The referenced entry always
    /// resolves (via the reachability) to a leaf — either an `Input` or a `Transmission` output.
    std::vector<BlueprintStateInterfaceId> sources{};
    /// Per-row affine coefficients. Identity (m=1, o=0) for direct input passthroughs.
    std::vector<double> multipliers{};
    std::vector<double> offsets{};

    // Pure-affine fast path: when the blueprint has no TransmissionStages, the planner
    // pre-resolves each source definition to its index in blueprint.inputs() and stores it
    // here. If non-empty, sources is empty and materialize_pure_affine reads these directly,
    // avoiding both the blueprint-SID lookup table and the O(N²) input-slot scan.
    std::vector<std::size_t> direct_input_slots{};

    // ---- Scratch-fill rows ----
    // Used for Case 3: a transmission input (real SID `r`) whose producer is an
    // AffineProjection from a bare source def. The bare source IS in scratch (user input),
    // but `r`'s def is NOT (neither user input nor transmission output). These rows pre-fill
    // `r`'s scratch slot so the downstream TransmissionStage gathers it normally.
    //
    // Emitted in the InputAffineBatch that PRECEDES the affected TransmissionStage.
    // Each row i: scratch[scratch_targets[i]] =
    //     scratch_multipliers[i] * scratch[scratch_sources[i]] + scratch_offsets[i].
    std::vector<BlueprintStateInterfaceId> scratch_targets{};
    std::vector<BlueprintStateInterfaceId> scratch_sources{};
    std::vector<double> scratch_multipliers{};
    std::vector<double> scratch_offsets{};
  };

  /// A single transmission instance to run. The materializer must compute the transmission's
  /// inputs (already in the value table at this point), execute the transmission, and place the
  /// outputs into the value table for downstream segments. For each transmission output, an
  /// optional `blueprint_output_indices` entry says whether (and where) that output should also
  /// be written to the blueprint's overall output buffer.
  struct TransmissionStage {
    /// The transmission instance id to execute.
    TransmissionInstanceId instance_id = 0;
    /// The transmission's input state interfaces (read from the value table at runtime).
    std::vector<BlueprintStateInterfaceId> inputs{};
    /// The transmission's output state interfaces (added to the value table at runtime).
    std::vector<BlueprintStateInterfaceId> outputs{};
    /// Parallel to `outputs`. If a transmission output is directly requested in the blueprint's
    /// `outputs()`, the corresponding entry holds the index in the blueprint's overall output
    /// buffer where that value should be written. Otherwise `std::nullopt` (the value is still
    /// computed and added to the value table for downstream consumers, but not exposed
    /// directly).
    std::vector<std::optional<std::size_t>> blueprint_output_indices{};
  };

  std::variant<InputAffineBatch, TransmissionStage> kind;
};

/**
 * The output of `plan_joint_map`: an immutable, ordered sequence of segments that, executed in
 * order, produces every requested output from the analyzed inputs.
 *
 * Builders walk `segments()` in order and emit one runtime segment per blueprint segment. The
 * affine-batching consolidation has already been done by `plan_joint_map`, so an
 * `InputAffineBatch` with N rows becomes one runtime `AffineJointMap` with N rows — not N
 * separate joint maps.
 */
class JointMapBlueprint {
public:
  JointMapBlueprint() = default;

  // Move-only.
  JointMapBlueprint(const JointMapBlueprint &) = delete;
  JointMapBlueprint & operator=(const JointMapBlueprint &) = delete;
  JointMapBlueprint(JointMapBlueprint &&) noexcept = default;
  JointMapBlueprint & operator=(JointMapBlueprint &&) noexcept = default;
  ~JointMapBlueprint() = default;

  /// The full segment list, in execution order. Walking this in order is sufficient to build a
  /// runtime joint map.
  [[nodiscard]] span<const JointMapBlueprintSegment> segments() const noexcept { return segments_; }

  /// Echo of the `inputs()` from the source `TransmissionReachability`. Stored here so consumers
  /// don't have to thread the reachability separately.
  [[nodiscard]] span<const StateInterfaceDefinition> inputs() const noexcept { return inputs_; }

  /// Echo of the `ordered_outputs` argument that was passed to `plan_joint_map`.
  [[nodiscard]] span<const StateInterfaceDefinition> outputs() const noexcept { return outputs_; }

  /// Blueprint-local canonical state-interface table. Segment-local ids index into this table.
  [[nodiscard]] span<const BlueprintStateInterfaceRecord> state_interfaces() const noexcept
  {
    return state_interface_records_;
  }

  // ---------------------------------------------------------------------------
  // Internal builder access
  // ---------------------------------------------------------------------------

  /// Used by `plan_joint_map`. Public so the free function can populate the blueprint without
  /// being a friend.
  void emplace_segment(JointMapBlueprintSegment segment) { segments_.push_back(std::move(segment)); }
  void set_inputs(std::vector<StateInterfaceDefinition> inputs) { inputs_ = std::move(inputs); }
  void set_outputs(std::vector<StateInterfaceDefinition> outputs) { outputs_ = std::move(outputs); }
  [[nodiscard]] BlueprintStateInterfaceId ensure_state_interface(BlueprintStateInterfaceRecord record)
  {
    const auto existing = state_interface_order_.contains_key(record.definition);
    const BlueprintStateInterfaceId local_id = state_interface_order_.ensure(record.definition);
    if (!existing) {
      state_interface_records_.push_back(std::move(record));
    }
    return local_id;
  }

  /// O(1) lookup of the blueprint-local id for a definition. Returns nullopt if the definition
  /// was never registered via ensure_state_interface (e.g., in pure-affine blueprints, or for
  /// definitions that are not used as transmission I/O or scratch sources).
  [[nodiscard]] std::optional<BlueprintStateInterfaceId> find_state_interface_id(
    const StateInterfaceDefinition & def) const noexcept
  {
    if (!state_interface_order_.contains_key(def)) {
      return std::nullopt;
    }
    return state_interface_order_[def];
  }

private:
  std::vector<JointMapBlueprintSegment> segments_{};
  std::vector<StateInterfaceDefinition> inputs_{};
  std::vector<StateInterfaceDefinition> outputs_{};
  Order<StateInterfaceDefinition, BlueprintStateInterfaceId> state_interface_order_{};
  std::vector<BlueprintStateInterfaceRecord> state_interface_records_{};
};

/**
 * Materializes a `TransmissionReachability` plus an ordered output list into an executable
 * blueprint. Pure function.
 *
 * The algorithm:
 * 1. Determine which transmissions are needed (walk back from each output that has a
 *    `Transmission` producer or an `AffineProjection` whose source is a transmission output).
 * 2. Topologically sort those transmissions (Kahn's algorithm).
 * 3. Assign each requested output to an execution stage:
 *    - `Input` producer or `AffineProjection` from an `Input` source → stage 0 (before any
 *      transmission runs).
 *    - `AffineProjection` from a `Transmission T_i` output → stage `i + 1` (after T_i runs).
 *    - `Transmission T_i` producer → emitted as part of T_i's `TransmissionStage`.
 * 4. Emit segments in order: `[InputAffineBatch_0] [TransmissionStage_0] [InputAffineBatch_1]
 *    [TransmissionStage_1] ...`. Empty `InputAffineBatch` segments are skipped. Each
 *    `TransmissionStage` is always emitted (even if no transmission outputs are directly
 *    requested) because downstream segments may need its outputs.
 *
 * \pre Every output in `ordered_outputs` must be safely producible — its producer chain
 *      (transitively, through transmission inputs and affine projection sources) must not
 *      touch any ambiguous interface in the reachability. Caller must run
 *      `diagnose_missing_outputs` first and only call this function when the diagnosis's
 *      `diagnose_missing_outputs(reach, ordered_outputs).ok()` returns true.
 *
 *      Unrelated ambiguities elsewhere in the reachability are allowed — only the requested
 *      outputs' chains matter. Violating this precondition asserts in debug builds; release
 *      behaviour is unspecified (likely a throw from the materializer).
 */
[[nodiscard]] JointMapBlueprint plan_joint_map(
  const TransmissionReachability & reach,
  span<const StateInterfaceDefinition> ordered_outputs);

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_JOINT_MAP_BLUEPRINT_HPP
