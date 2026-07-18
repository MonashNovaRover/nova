//
// Created by Bailey Chessum on 22/04/2026.
//

#include "arm_kinematics/joint_map/constant_supplemented_joint_map.hpp"

#include <cassert>

namespace arm_kinematics {

ConstantSupplementedJointMap::ConstantSupplementedJointMap(
  JointMap inner,
  std::vector<double> constants,
  const std::size_t real_count)
: inner_(std::move(inner)),
  real_count_(real_count),
  augmented_(inner_.input_count(), 0.0)
{
  assert(real_count_ + constants.size() == inner_.input_count());

  // Pre-fill the constant suffix once — never changed after construction.
  for (std::size_t i = 0; i < constants.size(); ++i) {
    augmented_[real_count_ + i] = constants[i];
  }
}

void ConstantSupplementedJointMap::map(
  const span<const double> inputs,
  const span<double> outputs) const
{
  // Copy caller's real inputs into the buffer prefix.
  for (std::size_t i = 0; i < real_count_; ++i) {
    augmented_[i] = inputs[i];
  }
  // The constant suffix is already baked in from construction.
  inner_.map({augmented_.data(), augmented_.size()}, outputs);
}

}  // namespace arm_kinematics
