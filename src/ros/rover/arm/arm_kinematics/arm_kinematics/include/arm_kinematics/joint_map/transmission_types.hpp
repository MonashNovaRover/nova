//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_TYPES_HPP
#define ARM_KINEMATICS_TRANSMISSION_TYPES_HPP

#include <cstddef>

namespace arm_kinematics {

// These usings are included to make types more explicit, as throwing size_t around everywhere with different meanings
// can get disorienting.

using JointId = std::size_t;
using StateInterfaceId = std::size_t;
using TransmissionInstanceId = std::size_t; ///< index of a TransmissionInstance inside a TransmissionAnalysis
using TransmissionModelId = std::size_t;

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_TYPES_HPP
