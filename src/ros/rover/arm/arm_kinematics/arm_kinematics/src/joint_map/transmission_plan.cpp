//
// Created by Bailey Chessum on 24/03/2026.
//

#include "arm_kinematics/joint_map/transmission_plan.hpp"

#include <algorithm>
#include <optional>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace arm_kinematics {

namespace {

std::string join_joint_ids(span<const JointId> joint_ids)
{
  std::ostringstream ss;
  auto first = true;
  for (const auto joint_id : joint_ids) {
    if (!first)
      ss << ", ";
    ss << joint_id;
    first = false;
  }
  return ss.str();
}

const std::vector<JointId> & consumed_joint_ids_for_direction(
  const TransmissionAnalysis::TransmissionInstance & transmission,
  const PropagationDirection direction)
{
  return direction == PropagationDirection::Forward ?
    transmission.input_joint_ids :
    transmission.output_joint_ids;
}

const std::vector<JointId> & produced_joint_ids_for_direction(
  const TransmissionAnalysis::TransmissionInstance & transmission,
  const PropagationDirection direction)
{
  return direction == PropagationDirection::Forward ?
    transmission.output_joint_ids :
    transmission.input_joint_ids;
}

std::optional<TransmissionPlanStage> make_direct_stage(
  const TransmissionAnalysis::TransmissionInstance & transmission,
  const TransmissionGroupId group_id,
  const TransmissionModel & model,
  const PropagationDirection direction,
  span<const JointId> requested_inputs,
  span<const JointId> requested_outputs,
  const JointQuantity quantity)
{
  const auto & consumed_joint_ids = consumed_joint_ids_for_direction(transmission, direction);
  const auto & produced_joint_ids = produced_joint_ids_for_direction(transmission, direction);

  if (!model.can_build(quantity, direction)) {
    return std::nullopt;
  }
  if (consumed_joint_ids != std::vector<JointId>(requested_inputs.begin(), requested_inputs.end())) {
    return std::nullopt;
  }
  if (produced_joint_ids != std::vector<JointId>(requested_outputs.begin(), requested_outputs.end())) {
    return std::nullopt;
  }

  return TransmissionPlanStage{
    group_id,
    direction,
    consumed_joint_ids,
    produced_joint_ids
  };
}

bool contains_all_joint_ids(
  const std::vector<JointId> & available_joint_ids,
  const std::vector<JointId> & required_joint_ids)
{
  return std::all_of(
    required_joint_ids.begin(),
    required_joint_ids.end(),
    [&available_joint_ids](const JointId joint_id) {
      return std::find(available_joint_ids.begin(), available_joint_ids.end(), joint_id) != available_joint_ids.end();
    });
}

std::vector<JointId> merge_joint_ids(
  const std::vector<JointId> & available_joint_ids,
  const std::vector<JointId> & produced_joint_ids)
{
  std::vector<JointId> merged_joint_ids = available_joint_ids;

  for (const auto joint_id : produced_joint_ids) {
    if (std::find(merged_joint_ids.begin(), merged_joint_ids.end(), joint_id) == merged_joint_ids.end()) {
      merged_joint_ids.push_back(joint_id);
    }
  }

  std::sort(merged_joint_ids.begin(), merged_joint_ids.end());
  return merged_joint_ids;
}

std::string join_joint_ids(const std::vector<JointId> & joint_ids)
{
  return join_joint_ids(span<const JointId>(joint_ids));
}

std::string available_joint_ids_key(const std::vector<JointId> & joint_ids)
{
  return join_joint_ids(joint_ids);
}

tl::expected<AffinePlanStage, std::string> make_affine_plan_stage_expected(
  const TransmissionAnalysis & analysis,
  const span<const JointId> input_joint_ids,
  const JointId output_joint_id)
{
  const auto input_it = std::find(input_joint_ids.begin(), input_joint_ids.end(), output_joint_id);
  if (input_it != input_joint_ids.end()) {
    return AffinePlanStage{
      output_joint_id,
      output_joint_id,
      static_cast<size_t>(input_it - input_joint_ids.begin()),
      1.0F,
      0.0F
    };
  }

  const auto & affine_transmissions = analysis.affine_transmissions();
  const auto affine_it = std::find_if(
    affine_transmissions.begin(),
    affine_transmissions.end(),
    [output_joint_id](const TransmissionAnalysis::AffineTransmission & affine_transmission) {
      return affine_transmission.target_joint_id == output_joint_id;
    });
  if (affine_it == affine_transmissions.end() && input_joint_ids.empty()) {
    return AffinePlanStage{
      output_joint_id,
      output_joint_id,
      0,
      0.0F,
      0.0F
    };
  }
  if (affine_it == affine_transmissions.end()) {
    return tl::make_unexpected("No affine plan found for output joint " + std::to_string(output_joint_id));
  }

  const auto duplicate_affine_it = std::find_if(
    std::next(affine_it),
    affine_transmissions.end(),
    [output_joint_id](const TransmissionAnalysis::AffineTransmission & affine_transmission) {
      return affine_transmission.target_joint_id == output_joint_id;
    });
  if (duplicate_affine_it != affine_transmissions.end()) {
    return tl::make_unexpected(
      "Ambiguous affine plan: multiple affine transmissions target joint " + std::to_string(output_joint_id));
  }

  const auto source_stage = make_affine_plan_stage_expected(analysis, input_joint_ids, affine_it->source_joint_id);
  if (!source_stage.has_value()) {
    return tl::make_unexpected(source_stage.error());
  }

  return AffinePlanStage{
    source_stage->source_joint_id,
    output_joint_id,
    source_stage->source_input_index,
    source_stage->multiplier * affine_it->multiplier,
    source_stage->offset * affine_it->multiplier + affine_it->offset
  };
}

} // namespace

MakeAffinePlanResult make_affine_plan_expected(
  const TransmissionAnalysis & analysis,
  const span<const JointId> input_joint_ids,
  const span<const JointId> output_joint_ids)
{
  AffinePlan plan{
    {input_joint_ids.begin(), input_joint_ids.end()},
    {output_joint_ids.begin(), output_joint_ids.end()},
    {}
  };
  plan.stages.reserve(output_joint_ids.size());

  for (const auto output_joint_id : output_joint_ids) {
    const auto stage = make_affine_plan_stage_expected(analysis, input_joint_ids, output_joint_id);
    if (!stage.has_value()) {
      return tl::make_unexpected(
        "No affine plan found for inputs [" + join_joint_ids(input_joint_ids) +
        "] and outputs [" + join_joint_ids(output_joint_ids) + "]: " + stage.error());
    }

    plan.stages.push_back(*stage);
  }

  return plan;
}

MakeTransmissionPlanResult make_transmission_plan_expected(
  const TransmissionAnalysis & analysis,
  const span<const JointId> input_joint_ids,
  const span<const JointId> output_joint_ids,
  const JointQuantity quantity)
{
  const auto & transmissions = analysis.transmissions();
  const auto & models = analysis.models();
  struct SearchState {
    std::vector<JointId> available_joint_ids{};
    std::vector<TransmissionPlanStage> stages{};
  };

  SearchState initial_state{
    {input_joint_ids.begin(), input_joint_ids.end()},
    {}
  };
  std::sort(initial_state.available_joint_ids.begin(), initial_state.available_joint_ids.end());

  std::queue<SearchState> queue{};
  queue.push(initial_state);

  std::unordered_map<std::string, size_t> visited_state_depths{};
  visited_state_depths.emplace(available_joint_ids_key(initial_state.available_joint_ids), 0u);
  std::unordered_map<std::string, size_t> state_path_counts{};
  state_path_counts.emplace(available_joint_ids_key(initial_state.available_joint_ids), 1u);

  std::optional<TransmissionPlan> plan = std::nullopt;

  while (!queue.empty()) {
    auto state = std::move(queue.front());
    queue.pop();
    const auto current_key = available_joint_ids_key(state.available_joint_ids);

    if (contains_all_joint_ids(state.available_joint_ids, {output_joint_ids.begin(), output_joint_ids.end()})) {
      if (state_path_counts[current_key] > 1 || plan.has_value()) {
        return tl::make_unexpected("Ambiguous transmission plan: multiple grouped candidates were found");
      }

      plan = TransmissionPlan{
        {input_joint_ids.begin(), input_joint_ids.end()},
        {output_joint_ids.begin(), output_joint_ids.end()},
        std::move(state.stages)
      };
      continue;
    }

    for (TransmissionGroupId group_id = 0; group_id < transmissions.size(); ++group_id) {
      const auto & transmission = transmissions[group_id];
      const auto & model = *models[transmission.model_id];

      for (const auto direction : {PropagationDirection::Forward, PropagationDirection::Reverse}) {
        if (!model.can_build(quantity, direction)) {
          continue;
        }

        const auto & consumed_joint_ids = consumed_joint_ids_for_direction(transmission, direction);
        const auto & produced_joint_ids = produced_joint_ids_for_direction(transmission, direction);
        if (!contains_all_joint_ids(state.available_joint_ids, consumed_joint_ids)) {
          continue;
        }

        const auto next_available_joint_ids = merge_joint_ids(state.available_joint_ids, produced_joint_ids);
        if (next_available_joint_ids == state.available_joint_ids) {
          continue;
        }

        const auto key = available_joint_ids_key(next_available_joint_ids);
        const auto next_depth = state.stages.size() + 1;
        const auto visited_it = visited_state_depths.find(key);
        if (visited_it == visited_state_depths.end()) {
          visited_state_depths.emplace(key, next_depth);
          state_path_counts.emplace(key, state_path_counts[current_key]);
        } else if (visited_it->second == next_depth) {
          state_path_counts[key] += state_path_counts[current_key];
          continue;
        } else {
          continue;
        }

        auto next_stages = state.stages;
        next_stages.push_back(TransmissionPlanStage{
          group_id,
          direction,
          consumed_joint_ids,
          produced_joint_ids
        });

        queue.push(SearchState{
          std::move(next_available_joint_ids),
          std::move(next_stages)
        });
      }
    }
  }

  if (!plan.has_value()) {
    return tl::make_unexpected(
      "No grouped transmission plan found for inputs [" + join_joint_ids(input_joint_ids) +
      "] and outputs [" + join_joint_ids(output_joint_ids) + "]");
  }

  return *plan;
}

MakeJointMapPlanResult make_joint_map_plan_expected(
  const TransmissionAnalysis & analysis,
  const span<const JointId> input_joint_ids,
  const span<const JointId> output_joint_ids,
  const JointQuantity quantity)
{
  std::vector<JointId> affine_output_joint_ids{};
  std::vector<size_t> affine_output_indices{};
  std::vector<JointId> grouped_output_joint_ids{};
  std::vector<size_t> grouped_output_indices{};

  affine_output_joint_ids.reserve(output_joint_ids.size());
  affine_output_indices.reserve(output_joint_ids.size());
  grouped_output_joint_ids.reserve(output_joint_ids.size());
  grouped_output_indices.reserve(output_joint_ids.size());

  for (size_t i = 0; i < output_joint_ids.size(); ++i) {
    const auto output_joint_id = output_joint_ids[i];
    const auto affine_stage = make_affine_plan_stage_expected(analysis, input_joint_ids, output_joint_id);
    if (affine_stage.has_value()) {
      affine_output_joint_ids.push_back(output_joint_id);
      affine_output_indices.push_back(i);
      continue;
    }

    grouped_output_joint_ids.push_back(output_joint_id);
    grouped_output_indices.push_back(i);
  }

  JointMapPlan plan{
    {input_joint_ids.begin(), input_joint_ids.end()},
    {output_joint_ids.begin(), output_joint_ids.end()},
    {}
  };

  if (!affine_output_joint_ids.empty()) {
    const auto affine_plan = make_affine_plan_expected(
      analysis,
      input_joint_ids,
      span<const JointId>(affine_output_joint_ids));
    if (!affine_plan.has_value()) {
      return tl::make_unexpected(affine_plan.error());
    }

    plan.segments.push_back(JointMapPlanSegment{
      std::move(affine_output_indices),
      *affine_plan
    });
  }

  if (!grouped_output_joint_ids.empty()) {
    const auto transmission_plan = make_transmission_plan_expected(
      analysis,
      input_joint_ids,
      span<const JointId>(grouped_output_joint_ids),
      quantity);
    if (!transmission_plan.has_value()) {
      return tl::make_unexpected(transmission_plan.error());
    }

    plan.segments.push_back(JointMapPlanSegment{
      std::move(grouped_output_indices),
      *transmission_plan
    });
  }

  if (plan.segments.empty()) {
    return tl::make_unexpected(
      "No joint map plan found for inputs [" + join_joint_ids(input_joint_ids) +
      "] and outputs [" + join_joint_ids(output_joint_ids) + "]");
  }

  return plan;
}

} // namespace arm_kinematics
