//
// Created by Bailey Chessum on 25/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_JOINT_MAP_HPP
#define ARM_KINEMATICS_TRANSMISSION_JOINT_MAP_HPP

#include <memory>
#include <vector>

#include "arm_kinematics/joint_map/compute_transmission.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_plan.hpp"
#include "arm_kinematics/joint_map/transmission_types.hpp"
#include "arm_kinematics/utilities/expected.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

struct CompiledTransmissionStage {
  std::unique_ptr<const ComputeTransmission> compute{};
  std::vector<size_t> input_indices{};
  std::vector<size_t> output_indices{};
  size_t scratch_offset = 0;
  size_t scratch_size = 0;

  CompiledTransmissionStage() = default;
  CompiledTransmissionStage(
    std::unique_ptr<const ComputeTransmission> compute,
    std::vector<size_t> input_indices,
    std::vector<size_t> output_indices,
    size_t scratch_offset,
    size_t scratch_size)
  : compute(std::move(compute)),
    input_indices(std::move(input_indices)),
    output_indices(std::move(output_indices)),
    scratch_offset(scratch_offset),
    scratch_size(scratch_size)
  {
  }
  CompiledTransmissionStage(const CompiledTransmissionStage & other);
  CompiledTransmissionStage(CompiledTransmissionStage &&) noexcept = default;
  CompiledTransmissionStage & operator=(const CompiledTransmissionStage & other);
  CompiledTransmissionStage & operator=(CompiledTransmissionStage &&) noexcept = default;
};

struct CompiledTransmissionPlan {
  size_t input_count = 0;
  size_t output_count = 0;
  size_t value_count = 0;
  size_t scratch_size = 0;
  std::vector<size_t> output_value_indices{};
  std::vector<CompiledTransmissionStage> stages{};

  CompiledTransmissionPlan() = default;
  CompiledTransmissionPlan(const CompiledTransmissionPlan &) = default;
  CompiledTransmissionPlan(CompiledTransmissionPlan &&) noexcept = default;
  CompiledTransmissionPlan & operator=(const CompiledTransmissionPlan &) = default;
  CompiledTransmissionPlan & operator=(CompiledTransmissionPlan &&) noexcept = default;
};

using CompileTransmissionPlanResult = tl::expected<CompiledTransmissionPlan, std::string>;

[[nodiscard]] ARM_KINEMATICS_PUBLIC CompileTransmissionPlanResult compile_transmission_plan_expected(
  const TransmissionAnalysis & analysis,
  const TransmissionPlan & transmission_plan,
  JointQuantity quantity);

class ARM_KINEMATICS_PUBLIC TransmissionJointMap {
public:
  explicit TransmissionJointMap(CompiledTransmissionPlan && compiled_plan);
  TransmissionJointMap(const TransmissionJointMap & other);
  TransmissionJointMap & operator=(const TransmissionJointMap & other);
  TransmissionJointMap(TransmissionJointMap &&) noexcept = default;
  TransmissionJointMap & operator=(TransmissionJointMap &&) noexcept = default;

  void map(span<const double> inputs, span<float> outputs) const;

  [[nodiscard]] size_t input_count() const noexcept { return compiled_plan_.input_count; }
  [[nodiscard]] size_t output_count() const noexcept { return compiled_plan_.output_count; }

private:
  struct Workspace {
    std::vector<float> values{};
    std::vector<float> stage_inputs{};
    std::vector<float> stage_outputs{};
    std::vector<float> scratch{};
  };

  [[nodiscard]] Workspace make_workspace() const;

  CompiledTransmissionPlan compiled_plan_{};
  mutable Workspace workspace_{};
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_JOINT_MAP_HPP
