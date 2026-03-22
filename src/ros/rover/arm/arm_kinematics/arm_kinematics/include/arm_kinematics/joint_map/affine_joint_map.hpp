//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_AFFINE_JOINT_MAP_HPP
#define ARM_KINEMATICS_AFFINE_JOINT_MAP_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <urdf/model.h>

#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_plan.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * Fast affine implementation of a JointMap.
 *
 * This preserves the original JointMap behavior:
 *   output[i] = input[sources_[i]] * multipliers_[i] + offsets_[i]
 *
 * It is primarily useful for:
 *   - caller-facing ordering -> compute-friendly ordering
 *   - duplication/fan-out of one input into multiple outputs
 *   - mimic-joint propagation
 *   - simple affine conversions
 */
class ARM_KINEMATICS_PUBLIC AffineJointMap {
public:
  AffineJointMap() = default;
  /**
   * Constructs an affine mapping from input_names to output_names, applying mimic-joint relationships where needed.
   */
  // AffineJointMap(
  //   const std::vector<std::string> & input_names,
  //   const std::vector<std::string> & output_names,
  //   const std::map<std::string, std::shared_ptr<urdf::JointMimic>> & mimic_joints = {});

  /**
   * Constructs an affine mapping from input_names to output_names using affine transmission relationships from the
   * given TransmissionAnalysis.
   */
  AffineJointMap(
    const std::vector<std::string> & input_names,
    const std::vector<std::string> & output_names,
    const TransmissionAnalysis & transmission_analysis);

  /**
   * Constructs an affine runtime map directly from a compiled affine plan.
   */
  explicit AffineJointMap(const AffinePlan & affine_plan);

  /// Creates an identity affine map for an N-element joint space.
  static AffineJointMap identity(size_t element_count);

  /**
   * \brief Maps all the given input joint positions to output joint positions using the affine gather mapping.
   * \note The values in inputs and outputs should correspond to the input and output name spaces used at construction.
   *
   * \param[in] inputs The position values for each input element in the source joint space.
   * \param[out] outputs The position values for each output element in the target joint space.
   *
   * \warning inputs and outputs must be pre-allocated to the correct size!
   * \warning inputs and outputs must not point to the same memory, or be any of the class's internal vectors.
   */
  void map(span<const double> inputs, span<float> outputs) const;

  /**
   * std::vector helper overload for map(span<const double>, span<float>).
   */
  void map(const std::vector<double> & inputs, std::vector<float> & outputs) const {
    map(
      {inputs.data(), inputs.size()},
      {outputs.data(), outputs.size()});
  }

  /// The number of elements in the input joint space.
  [[nodiscard]] size_t input_count() const noexcept { return input_count_; }
  /// The number of elements in the output joint space.
  [[nodiscard]] size_t output_count() const noexcept { return output_count_; }

private:
  /**
   * Recursively maps joint names to source indices while accumulating multiplier and offset values for any mimic
   * joints encountered.
   *
   * \param name The name of the output-space joint to resolve.
   * \param multiplier The multiplier accumulated from mimic joints.
   * \param offset The offset accumulated from mimic joints.
   * \returns The index of the input-space joint to source from.
   */
  static size_t find_source(
    const std::vector<std::string> & joint_names,
    std::map<std::string, std::shared_ptr<urdf::JointMimic>> mimic_joints,
    const std::string & name,
    float & multiplier,
    float & offset);

  /// output_count_ elements, the index of the input value to use for each output.
  std::vector<size_t> sources_{};
  /// output_count_ elements, the multiplier to apply to each sourced input.
  std::vector<float> multipliers_{};
  /// output_count_ elements, the offset to add to each sourced input.
  std::vector<float> offsets_{};
  /// The number of elements in the source joint space.
  size_t input_count_ = 0;
  /// The number of elements in the target joint space.
  size_t output_count_ = 0;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_AFFINE_JOINT_MAP_HPP
