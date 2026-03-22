//
// Created by Bailey Chessum on 25/03/2026.
//

#include "arm_kinematics/joint_map/transmission_joint_map.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>

namespace arm_kinematics {

namespace {

size_t ensure_value_index(
  std::unordered_map<JointId, size_t> & value_indices,
  const JointId joint_id,
  size_t & next_value_index)
{
  const auto it = value_indices.find(joint_id);
  if (it != value_indices.end()) {
    return it->second;
  }

  const auto value_index = next_value_index;
  value_indices.emplace(joint_id, value_index);
  ++next_value_index;
  return value_index;
}

std::vector<size_t> ensure_value_indices(
  std::unordered_map<JointId, size_t> & value_indices,
  span<const JointId> joint_ids,
  size_t & next_value_index)
{
  std::vector<size_t> indices{};
  indices.reserve(joint_ids.size());

  for (const auto joint_id : joint_ids) {
    indices.push_back(ensure_value_index(value_indices, joint_id, next_value_index));
  }

  return indices;
}

tl::expected<void, std::string> validate_stage_topology_expected(
  const TransmissionAnalysis::TransmissionInstance & transmission,
  const TransmissionPlanStage & stage)
{
  const auto & expected_consumed =
    stage.direction == PropagationDirection::Forward ?
    transmission.input_joint_ids :
    transmission.output_joint_ids;
  const auto & expected_produced =
    stage.direction == PropagationDirection::Forward ?
    transmission.output_joint_ids :
    transmission.input_joint_ids;

  if (stage.consumed_joint_ids != expected_consumed) {
    return tl::make_unexpected(
      "Transmission plan stage consumed joints do not match the referenced transmission topology");
  }
  if (stage.produced_joint_ids != expected_produced) {
    return tl::make_unexpected(
      "Transmission plan stage produced joints do not match the referenced transmission topology");
  }

  return {};
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
  compiled_plan.value_count = transmission_plan.input_joint_ids.size();
  compiled_plan.scratch_size = 0;
  compiled_plan.output_value_indices.reserve(transmission_plan.output_joint_ids.size());
  compiled_plan.stages.reserve(transmission_plan.stages.size());

  std::unordered_map<JointId, size_t> value_indices{};
  value_indices.reserve(
    transmission_plan.input_joint_ids.size() +
    transmission_plan.output_joint_ids.size());

  size_t next_value_index = 0;
  std::unordered_set<JointId> available_joint_ids{};
  available_joint_ids.reserve(
    transmission_plan.input_joint_ids.size() +
    transmission_plan.output_joint_ids.size());
  for (const auto joint_id : transmission_plan.input_joint_ids) {
    ensure_value_index(value_indices, joint_id, next_value_index);
    available_joint_ids.insert(joint_id);
  }

  const auto & models = analysis.models();
  for (const auto & stage : transmission_plan.stages) {
    if (stage.group_id >= analysis.transmissions().size()) {
      return tl::make_unexpected("Transmission plan references a group_id not present in TransmissionAnalysis");
    }

    const auto & transmission = analysis.transmissions()[stage.group_id];
    if (transmission.model_id >= models.size()) {
      return tl::make_unexpected("Transmission plan references a model_id not present in TransmissionAnalysis");
    }
    const auto stage_topology = validate_stage_topology_expected(transmission, stage);
    if (!stage_topology.has_value()) {
      return tl::make_unexpected(stage_topology.error());
    }
    for (const auto joint_id : stage.consumed_joint_ids) {
      if (available_joint_ids.find(joint_id) == available_joint_ids.end()) {
        return tl::make_unexpected(
          "Transmission plan stage consumes a joint that is not available from plan inputs or earlier stages");
      }
    }

    const auto input_indices = ensure_value_indices(
      value_indices,
      span<const JointId>(stage.consumed_joint_ids),
      next_value_index);
    const auto output_indices = ensure_value_indices(
      value_indices,
      span<const JointId>(stage.produced_joint_ids),
      next_value_index);

    if (!models[transmission.model_id]->can_build(quantity, stage.direction)) {
      return tl::make_unexpected(
        "Transmission plan stage references a transmission model that cannot build the requested direction/quantity");
    }

    auto compute = models[transmission.model_id]->build(
      quantity,
      stage.direction,
      span<const JointId>(stage.consumed_joint_ids),
      span<const JointId>(stage.produced_joint_ids));
    const auto stage_scratch_size = compute->scratch_size();

    compiled_plan.stages.push_back(CompiledTransmissionStage{
      std::move(compute),
      input_indices,
      output_indices,
      compiled_plan.scratch_size,
      stage_scratch_size
    });
    for (const auto joint_id : stage.produced_joint_ids) {
      available_joint_ids.insert(joint_id);
    }
    compiled_plan.scratch_size += stage_scratch_size;
  }

  for (const auto joint_id : transmission_plan.output_joint_ids) {
    const auto value_it = value_indices.find(joint_id);
    if (value_it == value_indices.end()) {
      return tl::make_unexpected(
        "Transmission plan output joint is not produced by the compiled grouped plan");
    }

    compiled_plan.output_value_indices.push_back(value_it->second);
  }
  compiled_plan.value_count = next_value_index;

  return compiled_plan;
}

TransmissionJointMap::TransmissionJointMap(CompiledTransmissionPlan && compiled_plan)
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
    std::vector<float>(compiled_plan_.value_count, 0.0F),
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

  auto & values = workspace_.values;
  for (size_t i = 0; i < inputs.size(); ++i) {
    values[i] = static_cast<float>(inputs[i]);
  }

  for (const auto & stage : compiled_plan_.stages) {
    for (size_t i = 0; i < stage.input_indices.size(); ++i) {
      workspace_.stage_inputs[i] = values[stage.input_indices[i]];
    }

    stage.compute->compute(
      span<const float>(workspace_.stage_inputs.data(), stage.input_indices.size()),
      span<float>(workspace_.stage_outputs.data(), stage.output_indices.size()),
      span<float>(
        workspace_.scratch.data() + stage.scratch_offset,
        stage.scratch_size));

    for (size_t i = 0; i < stage.output_indices.size(); ++i) {
      values[stage.output_indices[i]] = workspace_.stage_outputs[i];
    }
  }

  for (size_t i = 0; i < compiled_plan_.output_value_indices.size(); ++i) {
    outputs[i] = values[compiled_plan_.output_value_indices[i]];
  }
}

} // namespace arm_kinematics
