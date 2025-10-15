//
// Created by Bailey Chessum on 14/10/2025.
//

#include <arm_kinematics/joint_map.hpp>

namespace arm_kinematics {

JointMap::JointMap(const std::vector<std::string> & input_names, const std::vector<std::string> & output_names,
                   const std::map<std::string, std::shared_ptr<urdf::JointMimic>> & mimic_joints) {

  input_count = output_names.size();
  output_count = output_names.size();

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

    sources.emplace_back(source);
    multipliers.emplace_back(multiplier);
    offsets.emplace_back(offset);
  }
}

void JointMap::copy_values_to_jnts(const std::vector<double> & inputs, KDL::JntArray & jnts) const {
  if (inputs.size() != input_count) {
    throw std::runtime_error("Wrong number of elements in inputs");
    return;
  }
  if (jnts.rows() != input_count) {
    throw std::runtime_error("Wrong number of rows in jnts");
    return;
  }

  for (size_t i = 0; i < output_count; i++) {
    auto source = sources[i];
    jnts.data[static_cast<Eigen::Index>(i)] = multipliers[i] * inputs[source] + offsets[i];
  }
}

void JointMap::copy_values(const std::vector<double> & inputs, std::vector<double> & outputs) const {
  if (inputs.size() != input_count) {
    throw std::runtime_error("Wrong number of elements in inputs");
    return;
  }
  if (outputs.size() != input_count) {
    throw std::runtime_error("Wrong number of elements in outputs");
    return;
  }

  for (size_t i = 0; i < output_count; i++) {
    auto source = sources[i];
    outputs[i] = multipliers[i] * inputs[source] + offsets[i];
  }
}

size_t JointMap::find_source(const std::vector<std::string> & joint_names,
                             std::map<std::string, std::shared_ptr<urdf::JointMimic>> mimic_joints,
                             const std::string & name, double & multiplier, double & offset) {
  // Base case 1: If this joint is in joint_names, return its index
  auto names_it = std::find(joint_names.begin(), joint_names.end(), name);
  if (names_it != joint_names.begin()) {
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