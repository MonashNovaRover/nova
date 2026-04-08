//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/affine_joint_map.hpp"

#include <cassert>

namespace arm_kinematics {

AffineJointMap AffineJointMap::identity(const size_t element_count)
{
  AffineJointMap jm;
  jm.input_count_ = element_count;
  jm.output_count_ = element_count;
  jm.sources_.resize(element_count);
  jm.multipliers_.resize(element_count, 1.0F);
  jm.offsets_.resize(element_count, 0.0F);

  for (size_t i = 0; i < element_count; ++i) {
    jm.sources_[i] = i;
  }

  return jm;
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
