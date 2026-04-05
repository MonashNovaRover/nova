//
// Created by Bailey Chessum on 24/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_PLAN_HPP
#define ARM_KINEMATICS_TRANSMISSION_PLAN_HPP

#include <string>
#include <variant>
#include <vector>

#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/utilities/expected.hpp"

namespace arm_kinematics {

struct AffinePlanStage {
  StateInterfaceId source_joint_id = 0;
  StateInterfaceId target_joint_id = 0;
  size_t source_input_index = 0;
  float multiplier = 1.0F;
  float offset = 0.0F;
};

struct AffinePlan {
  std::vector<StateInterfaceId> input_joint_ids{};
  std::vector<StateInterfaceId> output_joint_ids{};
  std::vector<AffinePlanStage> stages{};
};

struct TransmissionPlanStage {
  TransmissionInstanceId transmission_instance_id = 0;
  std::vector<StateInterfaceId> consumed_joint_ids{};
  std::vector<StateInterfaceId> produced_joint_ids{};
};

struct TransmissionPlan {
  std::vector<StateInterfaceId> input_joint_ids{};
  std::vector<StateInterfaceId> output_joint_ids{};
  std::vector<TransmissionPlanStage> stages{};
};

using JointMapPlanSegmentPlan = std::variant<AffinePlan, TransmissionPlan>;

struct JointMapPlanSegment {
  std::vector<size_t> output_indices{};
  JointMapPlanSegmentPlan plan{};
};

struct JointMapPlanStage {
  std::vector<StateInterfaceId> input_joint_ids{};
  std::vector<StateInterfaceId> output_joint_ids{};
  std::vector<JointMapPlanSegment> segments{};
};

struct JointMapPlan {
  std::vector<StateInterfaceId> input_joint_ids{};
  std::vector<StateInterfaceId> output_joint_ids{};
  std::vector<JointMapPlanStage> stages{};
};

enum class JointMapPlanErrorKind
{
  NoPlan,
  Ambiguous,
  Invalid,
};

struct JointMapPlanError
{
  JointMapPlanErrorKind kind = JointMapPlanErrorKind::NoPlan;
  std::string message{};
};

enum class TransmissionPlanErrorKind
{
  NoPlan,
  Ambiguous,
  Invalid,
};

struct TransmissionPlanError
{
  TransmissionPlanErrorKind kind = TransmissionPlanErrorKind::NoPlan;
  std::string message{};
};

using MakeTransmissionPlanResult = tl::expected<TransmissionPlan, TransmissionPlanError>;
using MakeAffinePlanResult = tl::expected<AffinePlan, std::string>;
using MakeJointMapPlanResult = tl::expected<JointMapPlan, JointMapPlanError>;

[[nodiscard]] MakeAffinePlanResult make_affine_plan_expected(
  const TransmissionAnalysis & analysis,
  span<const StateInterfaceId> input_joint_ids,
  span<const StateInterfaceId> output_joint_ids);

[[nodiscard]] MakeTransmissionPlanResult make_transmission_plan_expected(
  const TransmissionAnalysis & analysis,
  span<const StateInterfaceId> input_joint_ids,
  span<const StateInterfaceId> output_joint_ids,
  JointQuantity quantity);

[[nodiscard]] MakeJointMapPlanResult make_joint_map_plan_expected(
  const TransmissionAnalysis & analysis,
  span<const StateInterfaceId> input_joint_ids,
  span<const StateInterfaceId> output_joint_ids,
  JointQuantity quantity);

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_PLAN_HPP
