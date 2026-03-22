//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_DEFINITION_HPP
#define ARM_KINEMATICS_TRANSMISSION_DEFINITION_HPP

#include <string>
#include <vector>

#include "arm_kinematics/joint_map/transmission_types.hpp"

namespace arm_kinematics {

template<typename TJoint>
struct TransmissionDefinition {
  std::vector<TJoint> inputs;
  std::vector<TJoint> outputs;

  using joint_type = TJoint;
};

using IndexedTransmissionDefinition = TransmissionDefinition<JointId>;

/// If you ever get a NamedTransmissionDefinition, immediately convert it to an IndexedTransmissionDefinition
using NamedTransmissionDefinition = TransmissionDefinition<std::string>;

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_DEFINITION_HPP
