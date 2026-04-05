//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/affine_joint_map.hpp"

#include <algorithm>

namespace arm_kinematics {

namespace {

std::vector<StateInterfaceId> to_joint_ids_checked(
  const Order<std::string, StateInterfaceId> & joint_order,
  const std::vector<std::string> & joint_names,
  const char * label)
{
  std::vector<StateInterfaceId> joint_ids{};
  joint_ids.reserve(joint_names.size());

  for (const auto & joint_name : joint_names) {
    if (!joint_order.contains_key(joint_name)) {
      throw std::runtime_error(
        std::string("No canonical JointId found for ") + label + " joint '" + joint_name + "'");
    }

    joint_ids.push_back(joint_order[joint_name]);
  }

  return joint_ids;
}

AffinePlan make_affine_plan_allowing_unknown_outputs(
  const TransmissionAnalysis & transmission_analysis,
  const std::vector<std::string> & input_names,
  const std::vector<std::string> & output_names)
{
  const auto & joint_order = transmission_analysis.state_interface_order();
  const auto input_joint_ids = to_joint_ids_checked(joint_order, input_names, "input");

  AffinePlan plan{};
  plan.input_joint_ids = input_joint_ids;
  plan.stages.reserve(output_names.size());
  plan.output_joint_ids.reserve(output_names.size());

  for (const auto & output_name : output_names) {
    if (!joint_order.contains_key(output_name)) {
      plan.output_joint_ids.push_back(0);
      plan.stages.push_back(AffinePlanStage{
        0,
        0,
        0,
        0.0F,
        0.0F
      });
      continue;
    }

    const auto output_joint_id = joint_order[output_name];
    const auto stage = make_affine_plan_expected(
      transmission_analysis,
      span<const StateInterfaceId>(input_joint_ids.data(), input_joint_ids.size()),
      span<const StateInterfaceId>(&output_joint_id, 1));
    if (!stage.has_value()) {
      throw std::runtime_error(stage.error());
    }

    plan.output_joint_ids.push_back(output_joint_id);
    plan.stages.push_back(stage->stages.front());
  }

  return plan;
}

} // namespace

// AffineJointMap::AffineJointMap(
//   const std::vector<std::string> & input_names,
//   const std::vector<std::string> & output_names,
//   const std::map<std::string, std::shared_ptr<urdf::JointMimic>> & mimic_joints)
//   : input_count_(input_names.size()),
//     output_count_(output_names.size())
// {
//   sources_.reserve(output_count_);
//   multipliers_.reserve(output_count_);
//   offsets_.reserve(output_count_);
//
//   for (const auto & name : output_names) {
//     float multiplier = 1.0F;
//     float offset = 0.0F;
//
//     const auto source = find_source(input_names, mimic_joints, name, multiplier, offset);
//
//     sources_.push_back(source);
//     multipliers_.push_back(multiplier);
//     offsets_.push_back(offset);
//   }
// }

AffineJointMap::AffineJointMap(
  const std::vector<std::string> & input_names,
  const std::vector<std::string> & output_names,
  const TransmissionAnalysis & transmission_analysis)
  : AffineJointMap(make_affine_plan_allowing_unknown_outputs(transmission_analysis, input_names, output_names))
{
}

AffineJointMap::AffineJointMap(const AffinePlan & affine_plan)
  : input_count_(affine_plan.input_joint_ids.size()),
    output_count_(affine_plan.output_joint_ids.size())
{
  sources_.reserve(output_count_);
  multipliers_.reserve(output_count_);
  offsets_.reserve(output_count_);

  for (const auto & stage : affine_plan.stages) {
    sources_.push_back(stage.source_input_index);
    multipliers_.push_back(stage.multiplier);
    offsets_.push_back(stage.offset);
  }
}

AffineJointMap AffineJointMap::identity(const size_t element_count)
{
  AffineJointMap jm;
  jm.input_count_ = element_count;
  jm.output_count_ = element_count;
  jm.sources_.resize(element_count);
  jm.multipliers_.resize(element_count, 1.0F);
  jm.offsets_.resize(element_count, 0.0F);

  for (size_t i = 0; i < element_count; ++i) {
    jm.sources_[i] = i;
  }

  return jm;
}

void AffineJointMap::map(span<const float> inputs, span<float> outputs) const
{
  assert(!input_count_ || inputs.size() == input_count_);
  assert(outputs.size() == output_count_);

  auto * __restrict__ out = outputs.data_;
  auto * __restrict__ off = offsets_.data();

  if (!input_count_) {
    #pragma omp simd
    for (size_t i = 0; i < output_count_; ++i) {
      out[i] = off[i];
    }
    return;
  }

  auto * __restrict__ in = inputs.data_;
  auto * __restrict__ mul = multipliers_.data();
  auto * __restrict__ src = sources_.data();

  for (size_t i = 0; i < output_count_; ++i) {
    out[i] = in[src[i]];
  }

  #pragma omp simd
  for (size_t i = 0; i < output_count_; ++i) {
    out[i] = out[i] * mul[i] + off[i];
  }
}

size_t AffineJointMap::find_source(
  const std::vector<std::string> & joint_names,
  const std::map<std::string, std::shared_ptr<urdf::JointMimic>> & mimic_joints,
  const std::string & name,
  float & multiplier,
  float & offset)
{
  const auto names_it = std::find(joint_names.begin(), joint_names.end(), name);
  if (names_it != joint_names.end()) {
    return static_cast<size_t>(names_it - joint_names.begin());
  }

  const auto mimic_it = mimic_joints.find(name);
  if (mimic_it == mimic_joints.end()) {
    // Keep current behavior: unresolved outputs evaluate to zero rather than failing the build.
    multiplier = 0.0F;
    offset = 0.0F;
    return 0;
  }

  const auto & mimic = mimic_it->second;

  offset += multiplier * static_cast<float>(mimic->offset);
  multiplier *= static_cast<float>(mimic->multiplier);

  return find_source(joint_names, mimic_joints, mimic->joint_name, multiplier, offset);
}

} // arm_kinematics
