//
// Created by Codex on 21/03/2026.
//

#ifndef ARM_KINEMATICS_COMPUTE_TRANSMISSION_HPP
#define ARM_KINEMATICS_COMPUTE_TRANSMISSION_HPP

#include <memory>

#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

class ARM_KINEMATICS_PUBLIC ComputeTransmission {
public:
  virtual ~ComputeTransmission() = default;

  virtual void compute(
    span<const float> inputs,
    span<float> outputs,
    span<float> scratch) const = 0;

  [[nodiscard]] virtual std::unique_ptr<ComputeTransmission> clone() const = 0;
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_COMPUTE_TRANSMISSION_HPP
