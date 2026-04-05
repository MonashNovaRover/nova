//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_MODEL_HPP
#define ARM_KINEMATICS_TRANSMISSION_MODEL_HPP

#include <memory>

#include "arm_kinematics/joint_map/compute_transmission.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

class ARM_KINEMATICS_PUBLIC TransmissionModel {
public:
  virtual ~TransmissionModel() = default;

  [[nodiscard]] virtual std::unique_ptr<TransmissionModel> clone() const = 0;

  [[nodiscard]] virtual std::unique_ptr<const ComputeTransmission> build(
    span<const StateInterfaceId> input_joint_ids,
    span<const StateInterfaceId> output_joint_ids) const = 0;
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_MODEL_HPP
