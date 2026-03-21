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
  std::string name;
  std::vector<TJoint> inputs;
  std::vector<TJoint> outputs;
  bool supports_forward = false;
  bool supports_reverse = false;

  using joint_type = TJoint;
};

using NamedTransmissionDefinition = TransmissionDefinition<std::string>;
using IndexedTransmissionDefinition = TransmissionDefinition<JointId>;

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_DEFINITION_HPP
