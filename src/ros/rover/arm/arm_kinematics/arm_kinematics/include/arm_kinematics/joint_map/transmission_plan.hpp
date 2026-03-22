//
// Created by Bailey Chessum on 24/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_PLAN_HPP
#define ARM_KINEMATICS_TRANSMISSION_PLAN_HPP

#include <string>
#include <vector>

#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/utilities/expected.hpp"

namespace arm_kinematics {

struct TransmissionPlanStage {
  TransmissionGroupId group_id = 0;
  PropagationDirection direction = PropagationDirection::Forward;
  std::vector<JointId> consumed_joint_ids{};
  std::vector<JointId> produced_joint_ids{};
};

struct TransmissionPlan {
  std::vector<JointId> input_joint_ids{};
  std::vector<JointId> output_joint_ids{};
  std::vector<TransmissionPlanStage> stages{};
};

using MakeTransmissionPlanResult = tl::expected<TransmissionPlan, std::string>;

[[nodiscard]] MakeTransmissionPlanResult make_transmission_plan_expected(
  const TransmissionAnalysis & analysis,
  span<const JointId> input_joint_ids,
  span<const JointId> output_joint_ids,
  JointQuantity quantity);

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_PLAN_HPP
