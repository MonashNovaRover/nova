//
// Created by Bailey Chessum on 22/04/2026.
//

#ifndef ARM_KINEMATICS_CONSTANT_SUPPLEMENTED_JOINT_MAP_HPP
#define ARM_KINEMATICS_CONSTANT_SUPPLEMENTED_JOINT_MAP_HPP

#include <cstddef>
#include <vector>

#include "arm_kinematics/joint_map/joint_map.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * Wraps a JointMap to supply constant values for a suffix of its input slots.
 *
 * The inner JointMap was built against an augmented input list:
 *   [ real user inputs (0 .. real_count-1) | constant inputs (real_count .. real_count+K-1) ]
 *
 * At runtime, callers only supply the real prefix. The constant suffix is pre-filled in an
 * internal buffer during construction and never changes. `map()` copies the caller's inputs
 * into the buffer prefix and then delegates to the inner map — no allocation at runtime.
 *
 * `input_count()` returns `real_count` (not the inner map's augmented count), so callers
 * correctly size their input vectors.
 */
class ARM_KINEMATICS_PUBLIC ConstantSupplementedJointMap {
public:
  /**
   * \param inner      JointMap built against the full augmented input list (real + constant slots).
   * \param constants  Values for the constant suffix. Length K must equal
   *                   inner.input_count() - real_count.
   * \param real_count Number of real user-supplied inputs.
   */
  ConstantSupplementedJointMap(JointMap inner, std::vector<double> constants, std::size_t real_count);

  void map(span<const double> inputs, span<double> outputs) const;

  [[nodiscard]] std::size_t input_count() const noexcept { return real_count_; }
  [[nodiscard]] std::size_t output_count() const noexcept { return inner_.output_count(); }

private:
  JointMap inner_;
  std::size_t real_count_;
  /// Pre-allocated buffer of size inner_.input_count(). Suffix (indices real_count_..end) is
  /// filled with the constants in the constructor and never modified afterward.
  mutable std::vector<double> augmented_;
};

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_CONSTANT_SUPPLEMENTED_JOINT_MAP_HPP
