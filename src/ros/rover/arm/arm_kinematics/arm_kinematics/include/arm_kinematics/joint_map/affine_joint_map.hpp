//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_AFFINE_JOINT_MAP_HPP
#define ARM_KINEMATICS_AFFINE_JOINT_MAP_HPP

#include <vector>

#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * Fast affine implementation of a JointMap.
 *
 * Runtime mapping is:
 * \code
 *   output[i] = input[sources_[i]] * multipliers_[i] + offsets_[i]
 * \endcode
 *
 * Useful for:
 *   - caller-facing ordering -> compute-friendly ordering
 *   - duplication / fan-out of one input into multiple outputs
 *   - mimic-joint propagation (with the mimic relationship pre-composed into multipliers/offsets)
 *   - simple affine conversions
 */
class ARM_KINEMATICS_PUBLIC AffineJointMap {
public:
  AffineJointMap() = default;

  /**
   * Direct constructor. Each output `i` is computed as:
   * \code
   *   outputs[i] = multipliers[i] * inputs[sources[i]] + offsets[i]
   * \endcode
   *
   * \param sources    Per-output index into the input vector. Length defines the output count.
   *                   Each entry must be `< input_count`.
   * \param multipliers  Same length as `sources`.
   * \param offsets    Same length as `sources`.
   * \param input_count  The size of the input vector this map will be called with at runtime.
   *                     May be larger than `max(sources)+1` (the map only reads the slots
   *                     referenced in `sources`).
   *
   * Throws `std::invalid_argument` if the parallel vectors are not the same length, or if any
   * source index is `>= input_count`.
   */
  AffineJointMap(
    std::vector<size_t> sources,
    std::vector<double> multipliers,
    std::vector<double> offsets,
    size_t input_count);

  /**
   * \brief Maps all the given input joint values to output joint values using the affine gather mapping.
   * \note The values in inputs and outputs should correspond to the input and output spaces used at construction.
   *
   * \param[in] inputs The values for each input element in the source joint space.
   * \param[out] outputs The values for each output element in the target joint space.
   *
   * \warning inputs and outputs must be pre-allocated to the correct size!
   * \warning inputs and outputs must not point to the same memory, or be any of the class's internal vectors.
   */
  void map(span<const double> inputs, span<double> outputs) const;

  /**
   * std::vector helper overload for map(span<const double>, span<double>).
   */
  void map(const std::vector<double> & inputs, std::vector<double> & outputs) const
  {
    map(
      {inputs.data(), inputs.size()},
      {outputs.data(), outputs.size()});
  }

  /// The number of elements in the input joint space.
  [[nodiscard]] size_t input_count() const noexcept { return input_count_; }
  /// The number of elements in the output joint space.
  [[nodiscard]] size_t output_count() const noexcept { return output_count_; }

private:
  /// output_count_ elements, the index of the input value to use for each output.
  std::vector<size_t> sources_{};
  /// output_count_ elements, the multiplier to apply to each sourced input.
  std::vector<double> multipliers_{};
  /// output_count_ elements, the offset to add to each sourced input.
  std::vector<double> offsets_{};
  /// The number of elements in the source joint space.
  size_t input_count_ = 0;
  /// The number of elements in the target joint space.
  size_t output_count_ = 0;
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_AFFINE_JOINT_MAP_HPP
