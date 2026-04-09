//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/affine_joint_map.hpp"

#include <cassert>
#include <stdexcept>
#include <utility>

namespace arm_kinematics {

AffineJointMap::AffineJointMap(
  std::vector<size_t> sources,
  std::vector<float> multipliers,
  std::vector<float> offsets,
  const size_t input_count)
  : sources_(std::move(sources)),
    multipliers_(std::move(multipliers)),
    offsets_(std::move(offsets)),
    input_count_(input_count),
    output_count_(sources_.size())
{
  if (multipliers_.size() != output_count_ || offsets_.size() != output_count_) {
    throw std::invalid_argument(
      "AffineJointMap: sources, multipliers, and offsets must have the same length");
  }
  for (const auto src : sources_) {
    if (src >= input_count_) {
      throw std::invalid_argument(
        "AffineJointMap: source index out of range for the declared input_count");
    }
  }
}

void AffineJointMap::map(span<const float> inputs, span<float> outputs) const
{
  assert(!input_count_ || inputs.size() == input_count_);
  assert(outputs.size() == output_count_);

  auto * __restrict__ out = outputs.data_;
  auto * __restrict__ off = offsets_.data();

  if (!input_count_) {
    #pragma omp simd
    for (size_t i = 0; i < output_count_; ++i) {
      out[i] = off[i];
    }
    return;
  }

  auto * __restrict__ in = inputs.data_;
  auto * __restrict__ mul = multipliers_.data();
  auto * __restrict__ src = sources_.data();

  for (size_t i = 0; i < output_count_; ++i) {
    out[i] = in[src[i]];
  }

  #pragma omp simd
  for (size_t i = 0; i < output_count_; ++i) {
    out[i] = out[i] * mul[i] + off[i];
  }
}

} // namespace arm_kinematics
