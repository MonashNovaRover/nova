//
// Created by Codex on 21/03/2026.
//

#include "arm_kinematics/joint_map/joint_map.hpp"

#include <stdexcept>

namespace arm_kinematics {

JointMap::JointMap(const JointMap & other)
{
  if (other.impl_) {
    impl_ = other.impl_->clone();
  }
}

JointMap & JointMap::operator=(const JointMap & other)
{
  if (this == &other)
    return *this;

  impl_.reset();
  if (other.impl_) {
    impl_ = other.impl_->clone();
  }

  return *this;
}

void JointMap::map(span<const double> inputs, span<float> outputs) const
{
  if (!impl_)
    throw std::logic_error("Used JointMap::map() on an invalid JointMap");

  impl_->map(inputs, outputs);
}

void JointMap::map(const std::vector<double> & inputs, std::vector<float> & outputs) const
{
  map(
    {inputs.data(), inputs.size()},
    {outputs.data(), outputs.size()});
}

size_t JointMap::input_count() const noexcept
{
  return impl_ ? impl_->input_count() : 0;
}

size_t JointMap::output_count() const noexcept
{
  return impl_ ? impl_->output_count() : 0;
}

} // arm_kinematics
