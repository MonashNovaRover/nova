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

std::string join_joint_ids(span<const StateInterfaceId> joint_ids)
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

JointMapPlanError make_joint_map_plan_error(const JointMapPlanErrorKind kind, std::string message)
{
  return JointMapPlanError{
    kind,
    std::move(message)
  };
}

TransmissionPlanError make_transmission_plan_error(const TransmissionPlanErrorKind kind, std::string message)
{
  return TransmissionPlanError{
    kind,
    std::move(message)
  };
}

const std::vector<StateInterfaceId> & consumed_joint_ids_for_direction(
  const TransmissionAnalysis::TransmissionInstance & transmission,
  const PropagationDirection direction)
{
  return direction == PropagationDirection::Forward ?
    transmission.input_ids :
    transmission.output_ids;
}

const std::vector<StateInterfaceId> & produced_joint_ids_for_direction(
  const TransmissionAnalysis::TransmissionInstance & transmission,
  const PropagationDirection direction)
{
  return direction == PropagationDirection::Forward ?
    transmission.output_ids :
    transmission.input_ids;
}

std::optional<TransmissionPlanStage> make_direct_stage(
  const TransmissionAnalysis::TransmissionInstance & transmission,
  const TransmissionInstanceId transmission_instance_id,
  const PropagationDirection direction,
  span<const StateInterfaceId> requested_inputs,
  span<const StateInterfaceId> requested_outputs,
  const JointQuantity)
{
  const auto & consumed_joint_ids = consumed_joint_ids_for_direction(transmission, direction);
  const auto & produced_joint_ids = produced_joint_ids_for_direction(transmission, direction);

  if (consumed_joint_ids != std::vector<StateInterfaceId>(requested_inputs.begin(), requested_inputs.end())) {
    return std::nullopt;
  }
  if (produced_joint_ids != std::vector<StateInterfaceId>(requested_outputs.begin(), requested_outputs.end())) {
    return std::nullopt;
  }

  return TransmissionPlanStage{
    transmission_instance_id,
    direction,
    consumed_joint_ids,
    produced_joint_ids
  };
}

bool contains_all_joint_ids(
  const std::vector<StateInterfaceId> & available_joint_ids,
  const std::vector<StateInterfaceId> & required_joint_ids)
{
  return std::all_of(
    required_joint_ids.begin(),
    required_joint_ids.end(),
    [&available_joint_ids](const StateInterfaceId joint_id) {
      return std::find(available_joint_ids.begin(), available_joint_ids.end(), joint_id) != available_joint_ids.end();
    });
}

std::vector<StateInterfaceId> merge_joint_ids(
  const std::vector<StateInterfaceId> & available_joint_ids,
  const std::vector<StateInterfaceId> & produced_joint_ids)
{
  std::vector<StateInterfaceId> merged_joint_ids = available_joint_ids;

  for (const auto joint_id : produced_joint_ids) {
    if (std::find(merged_joint_ids.begin(), merged_joint_ids.end(), joint_id) == merged_joint_ids.end()) {
      merged_joint_ids.push_back(joint_id);
    }
  }

  std::sort(merged_joint_ids.begin(), merged_joint_ids.end());
  return merged_joint_ids;
}

std::string join_joint_ids(const std::vector<StateInterfaceId> & joint_ids)
{
  return join_joint_ids(span<const StateInterfaceId>(joint_ids));
}

std::string available_joint_ids_key(const std::vector<StateInterfaceId> & joint_ids)
{
  return join_joint_ids(joint_ids);
}

tl::expected<AffinePlanStage, std::string> make_affine_plan_stage_expected(
  const TransmissionAnalysis & analysis,
  span<const StateInterfaceId> input_joint_ids,
  StateInterfaceId output_joint_id);

tl::expected<StateInterfaceId, std::string> find_affine_root_source_joint_id_expected(
  const TransmissionAnalysis & analysis,
  const StateInterfaceId output_joint_id)
{
  const auto & affine_transmissions = analysis.affine_transmissions();
  const auto affine_it = std::find_if(
    affine_transmissions.begin(),
    affine_transmissions.end(),
    [output_joint_id](const TransmissionAnalysis::AffineTransmission & affine_transmission) {
      return affine_transmission.target_id == output_joint_id;
    });
  if (affine_it == affine_transmissions.end()) {
    return output_joint_id;
  }

  const auto duplicate_affine_it = std::find_if(
    std::next(affine_it),
    affine_transmissions.end(),
    [output_joint_id](const TransmissionAnalysis::AffineTransmission & affine_transmission) {
      return affine_transmission.target_id == output_joint_id;
    });
  if (duplicate_affine_it != affine_transmissions.end()) {
    return tl::make_unexpected(
      "Ambiguous affine plan: multiple affine transmissions target joint " + std::to_string(output_joint_id));
  }

  return find_affine_root_source_joint_id_expected(analysis, affine_it->source_id);
}

tl::expected<std::vector<StateInterfaceId>, std::string> collect_affine_closure_joint_ids_expected(
  const TransmissionAnalysis & analysis,
  const span<const StateInterfaceId> input_joint_ids)
{
  std::vector<StateInterfaceId> closure_joint_ids{input_joint_ids.begin(), input_joint_ids.end()};
  std::sort(closure_joint_ids.begin(), closure_joint_ids.end());

  bool changed = true;
  while (changed) {
    changed = false;

    for (const auto & affine_transmission : analysis.affine_transmissions()) {
      const auto affine_stage = make_affine_plan_stage_expected(
        analysis,
        span<const StateInterfaceId>(closure_joint_ids),
        affine_transmission.target_id);
      if (!affine_stage.has_value()) {
        continue;
      }

      if (std::find(
        closure_joint_ids.begin(),
        closure_joint_ids.end(),
        affine_transmission.target_id) != closure_joint_ids.end()) {
        continue;
      }

      closure_joint_ids.push_back(affine_transmission.target_id);
      std::sort(closure_joint_ids.begin(), closure_joint_ids.end());
      changed = true;
    }
  }

  return closure_joint_ids;
}

tl::expected<AffinePlanStage, std::string> make_affine_plan_stage_expected(
  const TransmissionAnalysis & analysis,
  const span<const StateInterfaceId> input_joint_ids,
  const StateInterfaceId output_joint_id)
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
      return affine_transmission.target_id == output_joint_id;
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
      return affine_transmission.target_id == output_joint_id;
    });
  if (duplicate_affine_it != affine_transmissions.end()) {
    return tl::make_unexpected(
      "Ambiguous affine plan: multiple affine transmissions target joint " + std::to_string(output_joint_id));
  }

  const auto source_stage = make_affine_plan_stage_expected(analysis, input_joint_ids, affine_it->source_id);
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

JointMapPlanStage make_grouped_prefix_stage(
  const std::vector<StateInterfaceId> & input_joint_ids,
  const std::vector<StateInterfaceId> & output_joint_ids,
  const TransmissionPlanStage & grouped_stage)
{
  JointMapPlanStage stage{
    input_joint_ids,
    output_joint_ids,
    {}
  };

  std::vector<StateInterfaceId> identity_output_joint_ids{};
  std::vector<size_t> identity_output_indices{};
  identity_output_joint_ids.reserve(input_joint_ids.size());
  identity_output_indices.reserve(input_joint_ids.size());

  for (size_t i = 0; i < output_joint_ids.size(); ++i) {
    const auto output_joint_id = output_joint_ids[i];
    if (std::find(input_joint_ids.begin(), input_joint_ids.end(), output_joint_id) != input_joint_ids.end()) {
      identity_output_joint_ids.push_back(output_joint_id);
      identity_output_indices.push_back(i);
    }
  }

  if (!identity_output_joint_ids.empty()) {
    AffinePlan identity_plan{};
    identity_plan.input_joint_ids = input_joint_ids;
    identity_plan.output_joint_ids = identity_output_joint_ids;
    identity_plan.stages.reserve(identity_output_joint_ids.size());

    for (const auto output_joint_id : identity_output_joint_ids) {
      const auto input_it = std::find(input_joint_ids.begin(), input_joint_ids.end(), output_joint_id);
      identity_plan.stages.push_back(AffinePlanStage{
        output_joint_id,
        output_joint_id,
        static_cast<size_t>(input_it - input_joint_ids.begin()),
        1.0F,
        0.0F
      });
    }

    stage.segments.push_back(JointMapPlanSegment{
      std::move(identity_output_indices),
      std::move(identity_plan)
    });
  }

  std::vector<size_t> grouped_output_indices{};
  grouped_output_indices.reserve(grouped_stage.produced_joint_ids.size());
  for (const auto produced_joint_id : grouped_stage.produced_joint_ids) {
    const auto output_it = std::find(output_joint_ids.begin(), output_joint_ids.end(), produced_joint_id);
    grouped_output_indices.push_back(static_cast<size_t>(output_it - output_joint_ids.begin()));
  }

  TransmissionPlan grouped_plan{};
  grouped_plan.input_joint_ids = input_joint_ids;
  grouped_plan.output_joint_ids = grouped_stage.produced_joint_ids;
  grouped_plan.stages.push_back(grouped_stage);

  stage.segments.push_back(JointMapPlanSegment{
    std::move(grouped_output_indices),
    std::move(grouped_plan)
  });

  return stage;
}

} // namespace

MakeAffinePlanResult make_affine_plan_expected(
  const TransmissionAnalysis & analysis,
  const span<const StateInterfaceId> input_joint_ids,
  const span<const StateInterfaceId> output_joint_ids)
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
  const span<const StateInterfaceId> input_joint_ids,
  const span<const StateInterfaceId> output_joint_ids,
  const JointQuantity quantity)
{
  const auto & transmissions = analysis.transmissions();
  (void)quantity;
  struct SearchState {
    std::vector<StateInterfaceId> available_joint_ids{};
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
        return tl::make_unexpected(make_transmission_plan_error(
          TransmissionPlanErrorKind::Ambiguous,
          "Ambiguous transmission plan: multiple grouped candidates were found"));
      }

      plan = TransmissionPlan{
        {input_joint_ids.begin(), input_joint_ids.end()},
        {output_joint_ids.begin(), output_joint_ids.end()},
        std::move(state.stages)
      };
      continue;
    }

    for (TransmissionInstanceId transmission_instance_id = 0;
         transmission_instance_id < transmissions.size();
         ++transmission_instance_id) {
      const auto & transmission = transmissions[transmission_instance_id];

      for (const auto direction : {PropagationDirection::Forward, PropagationDirection::Reverse}) {
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
          transmission_instance_id,
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
    return tl::make_unexpected(make_transmission_plan_error(
      TransmissionPlanErrorKind::NoPlan,
      "No grouped transmission plan found for inputs [" + join_joint_ids(input_joint_ids) +
      "] and outputs [" + join_joint_ids(output_joint_ids) + "]"));
  }

  return *plan;
}

tl::expected<JointMapPlanStage, std::string> make_single_stage_joint_map_plan_expected(
  const TransmissionAnalysis & analysis,
  const span<const StateInterfaceId> input_joint_ids,
  const span<const StateInterfaceId> output_joint_ids,
  const JointQuantity quantity)
{
  std::vector<StateInterfaceId> affine_output_joint_ids{};
  std::vector<size_t> affine_output_indices{};
  std::vector<StateInterfaceId> grouped_output_joint_ids{};
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

  JointMapPlanStage stage_plan{
    {input_joint_ids.begin(), input_joint_ids.end()},
    {output_joint_ids.begin(), output_joint_ids.end()},
    {}
  };

  if (!affine_output_joint_ids.empty()) {
    const auto affine_plan = make_affine_plan_expected(
      analysis,
      input_joint_ids,
      span<const StateInterfaceId>(affine_output_joint_ids));
    if (!affine_plan.has_value()) {
      return tl::make_unexpected(affine_plan.error());
    }

    stage_plan.segments.push_back(JointMapPlanSegment{
      std::move(affine_output_indices),
      *affine_plan
    });
  }

  if (!grouped_output_joint_ids.empty()) {
    const auto transmission_plan = make_transmission_plan_expected(
      analysis,
      input_joint_ids,
      span<const StateInterfaceId>(grouped_output_joint_ids),
      quantity);
    if (!transmission_plan.has_value()) {
      return tl::make_unexpected(transmission_plan.error().message);
    }

    stage_plan.segments.push_back(JointMapPlanSegment{
      std::move(grouped_output_indices),
      *transmission_plan
    });
  }

  if (stage_plan.segments.empty()) {
    return tl::make_unexpected(
      "No joint map plan found for inputs [" + join_joint_ids(input_joint_ids) +
      "] and outputs [" + join_joint_ids(output_joint_ids) + "]");
  }

  return stage_plan;
}

std::vector<size_t> identity_output_indices(const size_t size)
{
  std::vector<size_t> indices{};
  indices.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    indices.push_back(i);
  }
  return indices;
}

size_t joint_map_plan_segment_cost(const JointMapPlanSegment & segment)
{
  if (const auto * affine_plan = std::get_if<AffinePlan>(&segment.plan)) {
    return affine_plan->stages.size();
  }

  const auto & transmission_plan = std::get<TransmissionPlan>(segment.plan);
  return transmission_plan.stages.size() * 4u;
}

size_t joint_map_plan_cost(const JointMapPlan & plan)
{
  size_t cost = plan.stages.size() * 16u;
  for (const auto & stage : plan.stages) {
    cost += stage.segments.size() * 8u;
    for (const auto & segment : stage.segments) {
      cost += joint_map_plan_segment_cost(segment);
    }
  }
  return cost;
}

std::string joint_map_plan_segment_signature(const JointMapPlanSegment & segment)
{
  std::ostringstream ss;
  ss << "idx[" << join_joint_ids(span<const size_t>(segment.output_indices)) << "]";

  if (const auto * affine_plan = std::get_if<AffinePlan>(&segment.plan)) {
    ss << "|affine|in[" << join_joint_ids(affine_plan->input_joint_ids) << "]";
    ss << "|out[" << join_joint_ids(affine_plan->output_joint_ids) << "]";
    for (const auto & affine_stage : affine_plan->stages) {
      ss << "|a(" << affine_stage.source_joint_id
         << "," << affine_stage.target_joint_id
         << "," << affine_stage.source_input_index
         << "," << affine_stage.multiplier
         << "," << affine_stage.offset << ")";
    }
    return ss.str();
  }

  const auto & transmission_plan = std::get<TransmissionPlan>(segment.plan);
  ss << "|grouped|in[" << join_joint_ids(transmission_plan.input_joint_ids) << "]";
  ss << "|out[" << join_joint_ids(transmission_plan.output_joint_ids) << "]";
  for (const auto & transmission_stage : transmission_plan.stages) {
    ss << "|t(" << transmission_stage.transmission_instance_id
       << "," << static_cast<int>(transmission_stage.direction)
       << ",c[" << join_joint_ids(transmission_stage.consumed_joint_ids)
       << "],p[" << join_joint_ids(transmission_stage.produced_joint_ids) << "])";
  }
  return ss.str();
}

std::string joint_map_plan_signature(const JointMapPlan & plan)
{
  std::ostringstream ss;
  ss << "plan|in[" << join_joint_ids(plan.input_joint_ids) << "]";
  ss << "|out[" << join_joint_ids(plan.output_joint_ids) << "]";
  for (const auto & stage : plan.stages) {
    ss << "|stage|in[" << join_joint_ids(stage.input_joint_ids) << "]";
    ss << "|out[" << join_joint_ids(stage.output_joint_ids) << "]";
    for (const auto & segment : stage.segments) {
      ss << "|" << joint_map_plan_segment_signature(segment);
    }
  }
  return ss.str();
}

void append_joint_map_plan_stages(
  std::vector<JointMapPlanStage> & stages,
  std::vector<JointMapPlanStage> appended_stages)
{
  for (auto & stage : appended_stages) {
    stages.push_back(std::move(stage));
  }
}

MakeJointMapPlanResult make_joint_map_plan_expected(
  const TransmissionAnalysis & analysis,
  const span<const StateInterfaceId> input_joint_ids,
  const span<const StateInterfaceId> output_joint_ids,
  const JointQuantity quantity)
{
  const auto single_stage_plan = make_single_stage_joint_map_plan_expected(
    analysis,
    input_joint_ids,
    output_joint_ids,
    quantity);
  if (single_stage_plan.has_value()) {
    return JointMapPlan{
      {input_joint_ids.begin(), input_joint_ids.end()},
      {output_joint_ids.begin(), output_joint_ids.end()},
      {std::move(*single_stage_plan)}
    };
  }

  std::optional<JointMapPlan> grouped_prefix_plan = std::nullopt;
  {
    const auto & transmissions = analysis.transmissions();
    std::vector<StateInterfaceId> current_input_joint_ids{input_joint_ids.begin(), input_joint_ids.end()};
    std::sort(current_input_joint_ids.begin(), current_input_joint_ids.end());

    for (TransmissionInstanceId transmission_instance_id = 0;
         transmission_instance_id < transmissions.size();
         ++transmission_instance_id) {
      const auto & transmission = transmissions[transmission_instance_id];

      for (const auto direction : {PropagationDirection::Forward, PropagationDirection::Reverse}) {

        const auto & consumed_joint_ids = consumed_joint_ids_for_direction(transmission, direction);
        const auto & produced_joint_ids = produced_joint_ids_for_direction(transmission, direction);
        if (!contains_all_joint_ids(current_input_joint_ids, consumed_joint_ids)) {
          continue;
        }

        const auto next_available_joint_ids = merge_joint_ids(current_input_joint_ids, produced_joint_ids);
        if (next_available_joint_ids == current_input_joint_ids) {
          continue;
        }

        const auto suffix_plan = make_joint_map_plan_expected(
          analysis,
          span<const StateInterfaceId>(next_available_joint_ids),
          output_joint_ids,
          quantity);
        if (!suffix_plan.has_value()) {
          if (suffix_plan.error().kind == JointMapPlanErrorKind::Ambiguous) {
            return tl::make_unexpected(suffix_plan.error());
          }
          continue;
        }

        std::vector<JointMapPlanStage> stages{};
        stages.push_back(make_grouped_prefix_stage(
          current_input_joint_ids,
          next_available_joint_ids,
          TransmissionPlanStage{
            transmission_instance_id,
            direction,
            consumed_joint_ids,
            produced_joint_ids
        }));
        append_joint_map_plan_stages(stages, std::move(suffix_plan->stages));

        if (grouped_prefix_plan.has_value()) {
          return tl::make_unexpected(make_joint_map_plan_error(
            JointMapPlanErrorKind::Ambiguous,
            "Ambiguous joint map plan: multiple staged grouped-prefix candidates were found"));
        }

        grouped_prefix_plan = JointMapPlan{
          {input_joint_ids.begin(), input_joint_ids.end()},
          {output_joint_ids.begin(), output_joint_ids.end()},
          std::move(stages)
        };
      }
    }
  }

  const auto affine_closure_joint_ids = collect_affine_closure_joint_ids_expected(analysis, input_joint_ids);
  if (!affine_closure_joint_ids.has_value()) {
    return tl::make_unexpected(make_joint_map_plan_error(
      JointMapPlanErrorKind::Invalid,
      affine_closure_joint_ids.error()));
  }

  std::optional<JointMapPlan> affine_prefix_plan_candidate = std::nullopt;
  if (*affine_closure_joint_ids != std::vector<StateInterfaceId>(input_joint_ids.begin(), input_joint_ids.end())) {
    const auto affine_prefix_plan = make_affine_plan_expected(
      analysis,
      input_joint_ids,
      span<const StateInterfaceId>(*affine_closure_joint_ids));
    if (!affine_prefix_plan.has_value()) {
      return tl::make_unexpected(make_joint_map_plan_error(
        JointMapPlanErrorKind::Invalid,
        affine_prefix_plan.error()));
    }

    const auto second_stage_plan = make_joint_map_plan_expected(
      analysis,
      span<const StateInterfaceId>(*affine_closure_joint_ids),
      output_joint_ids,
      quantity);
    if (!second_stage_plan.has_value()) {
      if (second_stage_plan.error().kind == JointMapPlanErrorKind::Ambiguous) {
        return tl::make_unexpected(second_stage_plan.error());
      }
    } else {
      std::vector<JointMapPlanStage> stages{};
      stages.push_back(JointMapPlanStage{
        {input_joint_ids.begin(), input_joint_ids.end()},
        *affine_closure_joint_ids,
        {
          JointMapPlanSegment{
            identity_output_indices(affine_closure_joint_ids->size()),
            *affine_prefix_plan
          }
        }
      });
      append_joint_map_plan_stages(stages, std::move(second_stage_plan->stages));

      affine_prefix_plan_candidate = JointMapPlan{
        {input_joint_ids.begin(), input_joint_ids.end()},
        {output_joint_ids.begin(), output_joint_ids.end()},
        std::move(stages)
      };
    }
  }

  if (grouped_prefix_plan.has_value() && affine_prefix_plan_candidate.has_value()) {
    const auto grouped_signature = joint_map_plan_signature(*grouped_prefix_plan);
    const auto affine_signature = joint_map_plan_signature(*affine_prefix_plan_candidate);
    if (grouped_signature == affine_signature) {
      return joint_map_plan_cost(*grouped_prefix_plan) <= joint_map_plan_cost(*affine_prefix_plan_candidate) ?
        *grouped_prefix_plan :
        *affine_prefix_plan_candidate;
    }

    return tl::make_unexpected(make_joint_map_plan_error(
      JointMapPlanErrorKind::Ambiguous,
      "Ambiguous joint map plan: competing grouped-prefix and affine-prefix staged candidates were found"));
  }
  if (grouped_prefix_plan.has_value()) {
    return *grouped_prefix_plan;
  }
  if (affine_prefix_plan_candidate.has_value()) {
    return *affine_prefix_plan_candidate;
  }

  std::vector<StateInterfaceId> final_affine_input_joint_ids{};
  final_affine_input_joint_ids.reserve(output_joint_ids.size());
  for (const auto output_joint_id : output_joint_ids) {
    const auto root_source_joint_id = find_affine_root_source_joint_id_expected(analysis, output_joint_id);
    if (!root_source_joint_id.has_value()) {
      return tl::make_unexpected(make_joint_map_plan_error(
        JointMapPlanErrorKind::Invalid,
        root_source_joint_id.error()));
    }

    if (std::find(
      final_affine_input_joint_ids.begin(),
      final_affine_input_joint_ids.end(),
      *root_source_joint_id) == final_affine_input_joint_ids.end()) {
      final_affine_input_joint_ids.push_back(*root_source_joint_id);
    }
  }

  if (final_affine_input_joint_ids != std::vector<StateInterfaceId>(output_joint_ids.begin(), output_joint_ids.end())) {
    const auto first_stage_plan = make_joint_map_plan_expected(
      analysis,
      input_joint_ids,
      span<const StateInterfaceId>(final_affine_input_joint_ids),
      quantity);
    if (!first_stage_plan.has_value()) {
      if (first_stage_plan.error().kind == JointMapPlanErrorKind::Ambiguous) {
        return tl::make_unexpected(first_stage_plan.error());
      }
    } else {
      const auto final_affine_plan = make_affine_plan_expected(
        analysis,
        span<const StateInterfaceId>(final_affine_input_joint_ids),
        output_joint_ids);
      if (!final_affine_plan.has_value()) {
        return tl::make_unexpected(make_joint_map_plan_error(
          JointMapPlanErrorKind::Invalid,
          final_affine_plan.error()));
      }

      std::vector<JointMapPlanStage> stages = std::move(first_stage_plan->stages);
      stages.push_back(JointMapPlanStage{
        final_affine_input_joint_ids,
        {output_joint_ids.begin(), output_joint_ids.end()},
        {
          JointMapPlanSegment{
            identity_output_indices(output_joint_ids.size()),
            *final_affine_plan
          }
        }
      });

      return JointMapPlan{
        {input_joint_ids.begin(), input_joint_ids.end()},
        {output_joint_ids.begin(), output_joint_ids.end()},
        std::move(stages)
      };
    }
  }

  return tl::make_unexpected(make_joint_map_plan_error(
    JointMapPlanErrorKind::NoPlan,
    single_stage_plan.error()));
}

} // namespace arm_kinematics
