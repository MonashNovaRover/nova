//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/transmission_analysis.hpp"

#include <stdexcept>

namespace arm_kinematics {

TransmissionModelId TransmissionAnalysis::add_model(std::unique_ptr<TransmissionModel> model)
{
  if (!model)
    throw std::invalid_argument("TransmissionAnalysis::add_model() received a null model");

  const auto model_id = models_.size();
  models_.push_back(std::move(model));
  return model_id;
}

void TransmissionAnalysis::add_transmission(
  const TransmissionModelId model_id,
  std::vector<JointId> && inputs,
  std::vector<JointId> && outputs,
  std::string name)
{
  if (model_id >= models_.size()) {
    throw std::invalid_argument(
      "TransmissionAnalysis::add_transmission() received a model_id that is not present in models()");
  }

  transmissions_.push_back(TransmissionInstance{
    model_id,
    std::move(inputs),
    std::move(outputs),
    std::move(name)
  });
}

void TransmissionAnalysis::add_transmission(
  const TransmissionModelId model_id,
  const span<const std::string> inputs,
  const span<const std::string> outputs,
  std::string name)
{
  std::vector<JointId> input_ids{};
  input_ids.reserve(inputs.size());
  for (const auto & input_name : inputs) {
    input_ids.emplace_back(ensure_joint_id(input_name));
  }

  std::vector<JointId> output_ids{};
  output_ids.reserve(outputs.size());
  for (const auto & output_name : outputs) {
    output_ids.emplace_back(ensure_joint_id(output_name));
  }

  add_transmission(
    model_id,
    std::move(input_ids),
    std::move(output_ids),
    std::move(name));
}

JointId TransmissionAnalysis::ensure_joint_id(const std::string & name)
{
  if (joint_order_.contains_key(name))
    return joint_order_[name];

  const auto id = joint_order_.inverse.size();
  joint_order_[name] = id;
  return id;
}

} // namespace arm_kinematics
