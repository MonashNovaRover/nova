//
// Created by Bailey Chessum on 25/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_JOINT_MAP_HPP
#define ARM_KINEMATICS_TRANSMISSION_JOINT_MAP_HPP

#include <memory>
#include <vector>

#include "arm_kinematics/joint_map/compute_transmission.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * Runtime joint map wrapping a single `ComputeTransmission`.
 *
 * The runtime mapping is:
 * \code
 *   gather:  tx_inputs[i]  = inputs[gather_indices[i]]   for i in [0, tx_input_count)
 *   compute: compute_->compute(tx_inputs, tx_outputs, scratch)
 *   copy:    outputs[i]    = tx_outputs[i]               for i in [0, tx_output_count)
 * \endcode
 *
 * The class owns its own per-instance scratch buffer (sized from `compute_->scratch_size()`)
 * so `map()` is allocation-free at runtime.
 *
 * For standalone use, pass `gather_indices = [0, 1, ..., tx_input_count - 1]` (i.e. the
 * identity gather) and `input_count = tx_input_count`. Inside a `CompositeJointMap`, the
 * `inputs` argument is the composite's scratch buffer, `input_count` equals the scratch size,
 * and `gather_indices` references arbitrary scratch slots.
 *
 * Outputs are written densely (positions 0..tx_output_count-1). The composite handles any
 * scatter into final positions or downstream scratch slots.
 */
class ARM_KINEMATICS_PUBLIC TransmissionJointMap {
public:
  TransmissionJointMap() = default;

  /**
   * \param compute        The transmission compute kernel. Must be non-null.
   * \param gather_indices  Per transmission-input slot, the index in the joint map's `inputs`
   *                        vector to read from. Length defines the transmission's input count.
   *                        Each entry must be `< input_count`.
   * \param tx_output_count  The number of outputs the transmission produces. Must equal the
   *                         compute's expected output count (the runtime cannot validate this
   *                         without invoking compute, so callers must size the output buffer
   *                         to this).
   * \param input_count    The size of the `inputs` vector this map will be called with at
   *                       runtime. May be larger than the transmission's input count.
   *
   * Throws `std::invalid_argument` if `compute` is null or any gather index is out of range.
   */
  TransmissionJointMap(
    std::unique_ptr<const ComputeTransmission> compute,
    std::vector<size_t> gather_indices,
    size_t tx_output_count,
    size_t input_count);

  ~TransmissionJointMap() = default;

  // Copyable: clones the underlying ComputeTransmission and reallocates scratch.
  TransmissionJointMap(const TransmissionJointMap & other);
  TransmissionJointMap & operator=(const TransmissionJointMap & other);
  TransmissionJointMap(TransmissionJointMap &&) noexcept = default;
  TransmissionJointMap & operator=(TransmissionJointMap &&) noexcept = default;

  void map(span<const float> inputs, span<float> outputs) const;

  [[nodiscard]] size_t input_count() const noexcept { return input_count_; }
  [[nodiscard]] size_t output_count() const noexcept { return output_count_; }
  [[nodiscard]] bool valid() const noexcept { return compute_ != nullptr; }

private:
  std::unique_ptr<const ComputeTransmission> compute_{};
  /// Length == transmission input count. Each entry indexes into the JointMap's `inputs`.
  std::vector<size_t> gather_indices_{};
  size_t input_count_ = 0;
  size_t output_count_ = 0;

  /// Per-instance reusable buffers. Sized at construction; never reallocated at map() time.
  /// Marked mutable so map() can be const while writing to them.
  mutable std::vector<float> tx_inputs_{};
  mutable std::vector<float> tx_outputs_{};
  mutable std::vector<float> scratch_{};
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_JOINT_MAP_HPP
