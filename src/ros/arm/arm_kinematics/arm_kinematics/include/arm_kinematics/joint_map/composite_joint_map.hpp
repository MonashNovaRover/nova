//
// Created by Bailey Chessum on 22/03/2026.
//

#ifndef ARM_KINEMATICS_COMPOSITE_JOINT_MAP_HPP
#define ARM_KINEMATICS_COMPOSITE_JOINT_MAP_HPP

#include <cstddef>
#include <optional>
#include <vector>

#include "arm_kinematics/joint_map/joint_map.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * One stage in a `CompositeJointMap` execution sequence.
 *
 * Each stage runs a `JointMap` (the segment) on the composite's *scratch buffer* (which holds
 * the value table — see `CompositeJointMap` doc), then scatters the segment's outputs to one
 * or both of:
 *   - other scratch slots, so downstream stages can read them
 *   - the composite's final output buffer, so the joint map's caller sees them
 *
 * The two scatter lists are parallel to the segment's outputs (length ==
 * `segment.output_count()`). For each output `i`:
 *   - if `scratch_scatter[i]` is set, the value is written to `scratch[*scratch_scatter[i]]`
 *   - if `output_scatter[i]` is set, the value is written to `outputs[*output_scatter[i]]`
 * Both, either, or neither may be set. (Neither is rare but legal — the value is dropped.)
 *
 * \note **The segment is expected to take its `inputs` argument as the composite's full
 * scratch buffer.** That means `segment.input_count()` must equal `CompositeJointMap`'s
 * `scratch_size`. Building the segments with the right `input_count` is the job of whoever
 * constructs the `CompositeJointMap` — typically `materialize_joint_map` walking a
 * `JointMapBlueprint`.
 */
struct CompositeJointMapStage {
  JointMap segment{};
  /// Per segment-output: optional scratch slot to write to. `std::nullopt` = skip.
  std::vector<std::optional<size_t>> scratch_scatter{};
  /// Per segment-output: optional final-output slot to write to. `std::nullopt` = skip.
  std::vector<std::optional<size_t>> output_scatter{};
};

/**
 * Runtime joint map that orchestrates multiple `JointMap` stages through a shared scratch
 * buffer (the value table).
 *
 * Lifecycle of a `map(inputs, outputs)` call:
 *   1. **Seed**: For each `(input_slot, scratch_slot)` in `input_seeds`, copy
 *      `scratch[scratch_slot] = inputs[input_slot]`. This populates the value table with the
 *      caller's inputs.
 *   2. **Stages**: For each stage in order, call `stage.segment.map(scratch, stage_workspace)`,
 *      then scatter `stage_workspace[i]` to `scratch[*stage.scratch_scatter[i]]` and/or
 *      `outputs[*stage.output_scatter[i]]` for each output `i`.
 *   3. **Final gather**: Apply any remaining `(scratch_slot, output_slot)` pairs in
 *      `final_output_gather` (typically used for inputs that pass through directly to outputs
 *      without a stage in between, plus stage outputs that bypass the per-stage scatter).
 *
 * Each stage segment is itself a `JointMap` whose `input_count` equals the composite's
 * `scratch_size`. The segments don't know about scratch — they just see a contiguous input
 * vector and produce a contiguous output vector. Whether their outputs land in the value
 * table, the final output, or both is decided by the composite's per-stage scatter lists.
 *
 * \note Not real-time safe to construct. Allocation-free at `map()` time once constructed.
 *
 * \note The `inputs` parameter to `map()` does NOT have to match the scratch size — `input_seeds`
 * dictates which input slots get copied where. This is what lets a composite have a small
 * external interface (e.g. 3 inputs) while running stages over a much larger value table.
 */
class ARM_KINEMATICS_PUBLIC CompositeJointMap {
public:
  CompositeJointMap() = default;

  /**
   * \param input_count    The size of the `inputs` vector this map will be called with at
   *                       runtime.
   * \param output_count   The size of the `outputs` vector this map will write at runtime.
   * \param scratch_size   The size of the value-table scratch buffer. Each stage's
   *                       `segment.input_count()` must equal this value.
   * \param input_seeds    Pairs of `(input_slot, scratch_slot)` applied at the start of
   *                       `map()`. Each `input_slot` must be `< input_count` and each
   *                       `scratch_slot` must be `< scratch_size`.
   * \param stages         Stages in execution order.
   * \param final_output_gather  Pairs of `(scratch_slot, output_slot)` applied after all
   *                             stages. Used for direct passthroughs and similar cases.
   *
   * Throws `std::invalid_argument` on shape mismatches.
   */
  CompositeJointMap(
    size_t input_count,
    size_t output_count,
    size_t scratch_size,
    std::vector<std::pair<size_t, size_t>> input_seeds,
    std::vector<CompositeJointMapStage> stages,
    std::vector<std::pair<size_t, size_t>> final_output_gather);

  void map(span<const double> inputs, span<double> outputs) const;

  [[nodiscard]] size_t input_count() const noexcept { return input_count_; }
  [[nodiscard]] size_t output_count() const noexcept { return output_count_; }
  [[nodiscard]] size_t scratch_size() const noexcept { return scratch_size_; }

private:
  struct Workspace {
    std::vector<double> scratch{};
    std::vector<std::vector<double>> stage_outputs{};
  };

  [[nodiscard]] Workspace make_workspace() const;

  size_t input_count_ = 0;
  size_t output_count_ = 0;
  size_t scratch_size_ = 0;
  std::vector<std::pair<size_t, size_t>> input_seeds_{};
  std::vector<CompositeJointMapStage> stages_{};
  std::vector<std::pair<size_t, size_t>> final_output_gather_{};

  mutable Workspace workspace_{};
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_COMPOSITE_JOINT_MAP_HPP
