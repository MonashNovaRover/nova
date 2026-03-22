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
  if (const auto * definition = model->indexed_definition()) {
    add_indexed_group(*definition, model_id);
  } else if (const auto * definition = model->named_definition()) {
    add_named_group(*definition, model_id);
  } else {
    throw std::invalid_argument(
      "TransmissionAnalysis::add_model() received a TransmissionModel without a named or indexed definition");
  }

  models_.push_back(std::move(model));
  return model_id;
}

void TransmissionAnalysis::add_transmission(
  const TransmissionModelId model_id,
  std::vector<JointId> && inputs,
  std::vector<JointId> && outputs,
  std::string name)
{
  // TODO

}

void TransmissionAnalysis::add_transmission(
  const TransmissionModelId model_id,
  const span<const std::string> && inputs,
  const span<const std::string> && outputs,
  std::string name)
{
  std::vector<JointId> inputIds;
  std::vector<JointId> outputIds;

  // TODO: fill above vectors with ensure_joint_id on inputs and outputs. remember to reserve the correct size.

  add_transmission(
    model_id,
    std::move(inputIds),
    std::move(outputIds),
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
