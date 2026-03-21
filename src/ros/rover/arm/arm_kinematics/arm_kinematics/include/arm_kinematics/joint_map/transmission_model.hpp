//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_MODEL_HPP
#define ARM_KINEMATICS_TRANSMISSION_MODEL_HPP

#include <memory>

#include "arm_kinematics/joint_map/compute_transmission.hpp"
#include "arm_kinematics/joint_map/transmission_definition.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

class ARM_KINEMATICS_PUBLIC TransmissionModel {
public:
  virtual ~TransmissionModel() = default;

  [[nodiscard]] virtual const NamedTransmissionDefinition & definition() const noexcept = 0;

  [[nodiscard]] virtual bool can_build(
    JointQuantity quantity,
    PropagationDirection direction) const noexcept = 0;

  [[nodiscard]] virtual std::unique_ptr<const ComputeTransmission> build(
    JointQuantity quantity,
    PropagationDirection direction,
    span<const JointId> input_joint_ids,
    span<const JointId> output_joint_ids) const = 0;
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_MODEL_HPP
