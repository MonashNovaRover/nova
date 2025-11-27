//
// Created by Bailey Chessum on 14/10/2025.
//

#include <arm_kinematics/joint_map/joint_map.hpp>

namespace arm_kinematics {

JointMap::JointMap(
  const std::vector<std::string> & input_names,
  const std::vector<std::string> & output_names,
  const std::map<std::string,
  std::shared_ptr<urdf::JointMimic>> & mimic_joints)
    : input_count(input_names.size()), output_count(output_names.size()) {

  sources.clear();
  multipliers.clear();
  offsets.clear();
  sources.reserve(output_count);
  multipliers.reserve(output_count);
  offsets.reserve(output_count);

  for (auto & name : output_names) {
    double multiplier = 1.0;
    double offset = 0.0;

    auto source = find_source(input_names, mimic_joints, name, multiplier, offset);

    sources.push_back(source);
    multipliers.push_back(multiplier);
    offsets.push_back(offset);
  }
}

void JointMap::map(const std::vector<double> & inputs, KDL::JntArray & jnts) const {
  assert(inputs.size() == input_count); //< Wrong number of elements in inputs
  assert(jnts.rows() == output_count);  //< Wrong number of elements in outputs

  // The Jeston Orin Nano should have FP64 NEON SIMD instructions
  // Thought I might try coax the compiler into using it

  // __restrict__ just tells the compiler that each memory address is unique here, so no dependencies exist
  auto* __restrict__ out = jnts.data.data();
  auto* __restrict__ in  = inputs.data();
  auto* __restrict__ mul = multipliers.data();
  auto* __restrict__ off = offsets.data();
  auto* __restrict__ src = sources.data();

  // Step 1 -- gather source inputs into outputs so it can be vectorized.
  // I can't find any vector gathers for the Orin, so I've split it into its own loop
  // I've reused out[] assuming the number of elements would easily fit in L1 cache.
  for (size_t i = 0; i < output_count; ++i) {
    auto source = src[i];
    out[i] = in[source];
  }

  // Step 2 -- Multiply and offset values, hopefully vectorized
#pragma omp simd
  for (size_t i = 0; i < output_count; ++i) {
    out[i] = out[i] * mul[i] + off[i];
  }
}

void JointMap::map(const std::vector<double> & inputs, std::vector<double> & outputs) const {
  assert(inputs.size() == input_count);   //< Wrong number of elements in inputs
  assert(outputs.size() == output_count); //< Wrong number of elements in outputs

  for (size_t i = 0; i < output_count; ++i) {
    outputs[i] = inputs[sources[i]] * multipliers[i] + offsets[i];
    RCLCPP_INFO_STREAM(rclcpp::get_logger("joe"), std::to_string(i) << " = " << multipliers[i] << " * (" << std::to_string(sources[i]) << ", " << std::to_string(inputs[sources[i]]) << ") + " << offsets[i] << " = " << std::to_string(outputs[i]));
  }
  return;

  // The Jeston Orin Nano should have FP64 NEON SIMD instructions
  // Thought I might try coax the compiler into using it

  auto* __restrict__ out = outputs.data();
  auto* __restrict__ in  = inputs.data();
  auto* __restrict__ mul = multipliers.data();
  auto* __restrict__ off = offsets.data();
  auto* __restrict__ src = sources.data();

  // Step 1 -- gather source inputs into outputs so it can be vectorized.
  // I can't find any vector gathers for the Orin, so I've split it into its own loop
  // I've reused out[] assuming the number of elements would easily fit in L1 cache.
  for (size_t i = 0; i < output_count; ++i) {
    auto source = src[i];
    out[i] = in[source];
  }

  // Step 2 -- Multiply and offset values, hopefully vectorized
  #pragma omp simd
  for (size_t i = 0; i < output_count; ++i) {
    out[i] = out[i] * mul[i] + off[i];
  }
}

size_t JointMap::find_source(const std::vector<std::string> & joint_names,
                             std::map<std::string, std::shared_ptr<urdf::JointMimic>> mimic_joints,
                             const std::string & name, double & multiplier, double & offset) {
  // Base case 1: If this joint is in joint_names, return its index
  auto names_it = std::find(joint_names.begin(), joint_names.end(), name);
  if (names_it != joint_names.end()) {
    return names_it - joint_names.begin();
  }

  // Base case 2: This joint is not a mimic joint nor a joint in joint_names.
  auto mimic_it = mimic_joints.find(name);
  if (mimic_it == mimic_joints.end()) {
    // Make it always evaluate to 0
    multiplier = 0;
    offset = 0;
    return 0;
  }

  // Otherwise, if this is a mimic joint, call recursively until we find the source
  auto & mimic = mimic_it->second;

  offset += multiplier * mimic->offset;
  multiplier *= mimic->multiplier;

  return find_source(joint_names, mimic_joints, mimic->joint_name, multiplier, offset);
}

} // arm_kinematics