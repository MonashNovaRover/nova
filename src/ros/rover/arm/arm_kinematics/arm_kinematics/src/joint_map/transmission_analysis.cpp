//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/transmission_analysis.hpp"

#include <stdexcept>

namespace arm_kinematics {

ModelId TransmissionAnalysis::add_model(std::unique_ptr<TransmissionModel> model)
{
  if (!model)
    throw std::invalid_argument("TransmissionAnalysis::add_model() received a null model");

  const auto model_id = models_.size();
  const auto & definition = model->definition();

  Group group{};
  group.model_id = model_id;
  group.supports_forward = definition.supports_forward;
  group.supports_reverse = definition.supports_reverse;

  group.input_joint_ids.reserve(definition.inputs.size());
  for (const auto & input_name : definition.inputs) {
    group.input_joint_ids.emplace_back(ensure_joint_id(input_name));
  }

  group.output_joint_ids.reserve(definition.outputs.size());
  for (const auto & output_name : definition.outputs) {
    group.output_joint_ids.emplace_back(ensure_joint_id(output_name));
  }

  models_.push_back(std::move(model));
  groups_.push_back(std::move(group));

  return model_id;
}

bool TransmissionAnalysis::contains_joint(const std::string & name) const noexcept
{
  return joint_ids_.find(name) != joint_ids_.end();
}

std::optional<JointId> TransmissionAnalysis::find_joint_id(const std::string & name) const noexcept
{
  const auto it = joint_ids_.find(name);
  if (it == joint_ids_.end())
    return std::nullopt;

  return it->second;
}

JointId TransmissionAnalysis::ensure_joint_id(const std::string & name)
{
  if (const auto existing = find_joint_id(name))
    return *existing;

  const auto id = joint_names_.size();
  joint_names_.push_back(name);
  joint_ids_.emplace(name, id);
  return id;
}

} // namespace arm_kinematics
