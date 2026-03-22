//
// Created by Bailey Chessum on 25/03/2026.
//

#include "arm_kinematics/joint_map/transmission_joint_map.hpp"

#include <algorithm>
#include <stdexcept>

namespace arm_kinematics {

namespace {

tl::expected<std::vector<size_t>, std::string> make_indices_expected(
  const std::vector<JointId> & plan_joint_ids,
  const std::vector<JointId> & stage_joint_ids,
  const char * label)
{
  std::vector<size_t> indices{};
  indices.reserve(stage_joint_ids.size());

  for (const auto joint_id : stage_joint_ids) {
    const auto joint_it = std::find(plan_joint_ids.begin(), plan_joint_ids.end(), joint_id);
    if (joint_it == plan_joint_ids.end()) {
      return tl::make_unexpected(
        std::string("Transmission plan stage references a ") + label +
        " joint that is not present in the enclosing plan");
    }

    indices.push_back(static_cast<size_t>(joint_it - plan_joint_ids.begin()));
  }

  return indices;
}

} // namespace

CompileTransmissionPlanResult compile_transmission_plan_expected(
  const TransmissionAnalysis & analysis,
  const TransmissionPlan & transmission_plan,
  const JointQuantity quantity)
{
  CompiledTransmissionPlan compiled_plan{};
  compiled_plan.input_count = transmission_plan.input_joint_ids.size();
  compiled_plan.output_count = transmission_plan.output_joint_ids.size();
  compiled_plan.scratch_size = 0;
  compiled_plan.stages.reserve(transmission_plan.stages.size());

  const auto & models = analysis.models();
  for (const auto & stage : transmission_plan.stages) {
    if (stage.group_id >= analysis.transmissions().size()) {
      return tl::make_unexpected("Transmission plan references a group_id not present in TransmissionAnalysis");
    }

    const auto & transmission = analysis.transmissions()[stage.group_id];
    if (transmission.model_id >= models.size()) {
      return tl::make_unexpected("Transmission plan references a model_id not present in TransmissionAnalysis");
    }

    auto input_indices = make_indices_expected(
      transmission_plan.input_joint_ids,
      stage.consumed_joint_ids,
      "consumed");
    if (!input_indices.has_value()) {
      return tl::make_unexpected(input_indices.error());
    }

    auto output_indices = make_indices_expected(
      transmission_plan.output_joint_ids,
      stage.produced_joint_ids,
      "produced");
    if (!output_indices.has_value()) {
      return tl::make_unexpected(output_indices.error());
    }

    auto compute = models[transmission.model_id]->build(
      quantity,
      stage.direction,
      span<const JointId>(stage.consumed_joint_ids),
      span<const JointId>(stage.produced_joint_ids));

    compiled_plan.stages.push_back(CompiledTransmissionStage{
      std::move(compute),
      std::move(*input_indices),
      std::move(*output_indices),
      compiled_plan.scratch_size,
      0
    });
  }

  return compiled_plan;
}

TransmissionJointMap::TransmissionJointMap(CompiledTransmissionPlan compiled_plan)
  : compiled_plan_(std::move(compiled_plan))
{
  workspace_ = make_workspace();
}

TransmissionJointMap::Workspace TransmissionJointMap::make_workspace() const
{
  size_t max_stage_input_count = 0;
  size_t max_stage_output_count = 0;
  for (const auto & stage : compiled_plan_.stages) {
    max_stage_input_count = std::max(max_stage_input_count, stage.input_indices.size());
    max_stage_output_count = std::max(max_stage_output_count, stage.output_indices.size());
  }

  return Workspace{
    std::vector<float>(compiled_plan_.input_count, 0.0F),
    std::vector<float>(max_stage_input_count, 0.0F),
    std::vector<float>(max_stage_output_count, 0.0F),
    std::vector<float>(compiled_plan_.scratch_size, 0.0F)
  };
}

void TransmissionJointMap::map(span<const double> inputs, span<float> outputs) const
{
  if (inputs.size() != compiled_plan_.input_count) {
    throw std::invalid_argument("TransmissionJointMap::map() received inputs with the wrong size");
  }
  if (outputs.size() != compiled_plan_.output_count) {
    throw std::invalid_argument("TransmissionJointMap::map() received outputs with the wrong size");
  }

  auto & input_buffer = workspace_.inputs;
  for (size_t i = 0; i < inputs.size(); ++i) {
    input_buffer[i] = static_cast<float>(inputs[i]);
  }

  for (const auto & stage : compiled_plan_.stages) {
    for (size_t i = 0; i < stage.input_indices.size(); ++i) {
      workspace_.stage_inputs[i] = input_buffer[stage.input_indices[i]];
    }

    stage.compute->compute(
      span<const float>(workspace_.stage_inputs.data(), stage.input_indices.size()),
      span<float>(workspace_.stage_outputs.data(), stage.output_indices.size()),
      span<float>(
        workspace_.scratch.data() + stage.scratch_offset,
        stage.scratch_size));

    for (size_t i = 0; i < stage.output_indices.size(); ++i) {
      outputs[stage.output_indices[i]] = workspace_.stage_outputs[i];
    }
  }
}

} // namespace arm_kinematics
