//
// Created by Bailey Chessum on 25/03/2026.
//

#include "arm_kinematics/joint_map/transmission_joint_map.hpp"

#include <cassert>
#include <stdexcept>
#include <utility>

namespace arm_kinematics {

TransmissionJointMap::TransmissionJointMap(
  std::unique_ptr<const ComputeTransmission> compute,
  std::vector<size_t> gather_indices,
  const size_t tx_output_count,
  const size_t input_count)
  : compute_(std::move(compute)),
    gather_indices_(std::move(gather_indices)),
    input_count_(input_count),
    output_count_(tx_output_count)
{
  if (!compute_) {
    throw std::invalid_argument("TransmissionJointMap: compute must not be null");
  }
  for (const auto idx : gather_indices_) {
    if (idx >= input_count_) {
      throw std::invalid_argument(
        "TransmissionJointMap: gather index out of range for the declared input_count");
    }
  }
  tx_inputs_.assign(gather_indices_.size(), 0.0);
  tx_outputs_.assign(output_count_, 0.0);
  scratch_.assign(compute_->scratch_size(), 0.0);
}

TransmissionJointMap::TransmissionJointMap(const TransmissionJointMap & other)
  : compute_(other.compute_ ? other.compute_->clone() : nullptr),
    gather_indices_(other.gather_indices_),
    input_count_(other.input_count_),
    output_count_(other.output_count_),
    tx_inputs_(other.tx_inputs_.size(), 0.0),
    tx_outputs_(other.tx_outputs_.size(), 0.0),
    scratch_(other.scratch_.size(), 0.0)
{
}

TransmissionJointMap & TransmissionJointMap::operator=(const TransmissionJointMap & other)
{
  if (this == &other) {
    return *this;
  }
  compute_ = other.compute_ ? other.compute_->clone() : nullptr;
  gather_indices_ = other.gather_indices_;
  input_count_ = other.input_count_;
  output_count_ = other.output_count_;
  tx_inputs_.assign(other.tx_inputs_.size(), 0.0);
  tx_outputs_.assign(other.tx_outputs_.size(), 0.0);
  scratch_.assign(other.scratch_.size(), 0.0);
  return *this;
}

void TransmissionJointMap::map(
  const span<const double> inputs,
  const span<double> outputs) const
{
  assert(inputs.size() == input_count_);
  assert(outputs.size() == output_count_);
  assert(compute_ && "TransmissionJointMap::map() called on a default-constructed instance");

  // Gather: pull each transmission input from the joint map's input vector.
  const double * __restrict__ in = inputs.data_;
  for (size_t i = 0; i < gather_indices_.size(); ++i) {
    tx_inputs_[i] = in[gather_indices_[i]];
  }

  // Compute.
  compute_->compute(
    span<const double>(tx_inputs_.data(), tx_inputs_.size()),
    span<double>(tx_outputs_.data(), tx_outputs_.size()),
    span<double>(scratch_.data(), scratch_.size()));

  // Copy outputs densely. The composite (if any) handles further scatter into final positions
  // or downstream scratch slots.
  double * __restrict__ out = outputs.data_;
  for (size_t i = 0; i < output_count_; ++i) {
    out[i] = tx_outputs_[i];
  }
}

} // namespace arm_kinematics
