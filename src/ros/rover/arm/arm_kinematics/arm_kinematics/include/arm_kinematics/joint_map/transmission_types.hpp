//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_TYPES_HPP
#define ARM_KINEMATICS_TRANSMISSION_TYPES_HPP

#include <cstddef>

namespace arm_kinematics {

using JointId = std::size_t;
using TransmissionGroupId = std::size_t;
using TransmissionModelId = std::size_t;

enum class JointQuantity {
  Position,
  Velocity,
};

enum class PropagationDirection {
  Forward,
  Reverse,
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_TYPES_HPP
