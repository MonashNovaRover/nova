//
// Created by Bailey Chessum on 25/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_JOINT_MAP_HPP
#define ARM_KINEMATICS_TRANSMISSION_JOINT_MAP_HPP

#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * Runtime joint map for transmission-backed propagation.
 *
 * \note This class is a stub during step 2 of the state-interface refactor. The legacy
 * `CompiledTransmissionPlan` / `CompiledTransmissionStage` machinery and the
 * `compile_transmission_plan_expected` helper have been deleted; the new direct
 * constructor / factory that takes `unique_ptr<const ComputeTransmission>` instances
 * (built by walking a `JointMapBlueprint`) will be added in step 5. Until then this
 * class has no usable construction path.
 */
class ARM_KINEMATICS_PUBLIC TransmissionJointMap {
public:
  TransmissionJointMap() = default;
  TransmissionJointMap(const TransmissionJointMap &) = default;
  TransmissionJointMap(TransmissionJointMap &&) noexcept = default;
  TransmissionJointMap & operator=(const TransmissionJointMap &) = default;
  TransmissionJointMap & operator=(TransmissionJointMap &&) noexcept = default;
  ~TransmissionJointMap() = default;
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_JOINT_MAP_HPP
