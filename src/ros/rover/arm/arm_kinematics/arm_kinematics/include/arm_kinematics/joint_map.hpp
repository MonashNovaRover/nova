//
// Created by Bailey Chessum on 14/10/2025.
//

#ifndef ARM_KINEMATICS_JOINT_MAP_HPP
#define ARM_KINEMATICS_JOINT_MAP_HPP

#include <cstddef>
#include <urdf/model.h>
#include <kdl_parser/kdl_parser.hpp>
#include <kdl/jntarray.hpp>
#include <rclcpp/logger.hpp>
#include <rclcpp/logging.hpp>


namespace arm_kinematics {

// TODO: Allow a mimic joint name to be used as an input, where the joint the mimic joint references is calculated as
//       the inverse of that mimic joint in outputs.
// TODO: Support Transmissions. This will probably involve moving away from the sources, multipliers, offsets approach.

/**
 * Helper class to map between the parameterized set of joints and the KDL chain/JntArray joints.
 */
class JointMap {
public:
  JointMap() = default;
  JointMap(const std::vector<std::string>& input_names,
           const std::vector<std::string>& output_names,
           const std::map<std::string, std::shared_ptr<urdf::JointMimic>> & mimic_joints = {}) {

    input_count = output_names.size();
    output_count = output_names.size();

    sources.clear();
    sources.reserve(output_count);

    for (auto & name : output_names) {
      double multiplier = 1.0;
      double offset = 0.0;

      auto source = find_source(input_names, mimic_joints, name, multiplier, offset);

      sources.emplace_back(source);
      multipliers.emplace_back(multiplier);
      offsets.emplace_back(offset);
    }
  }

  void copy_values_to_jnts(const std::vector<double> & inputs, KDL::JntArray & jnts) const {
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

  void copy_values(const std::vector<double> & inputs, std::vector<double> & outputs) const {
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

  std::vector<size_t> sources{};
  std::vector<double> multipliers{};
  std::vector<double> offsets{};

  size_t input_count = 0;
  size_t output_count = 0;

  std::vector<std::string> output_names{};

private:
  /**
   * Recursively maps joint names to source value ids (where joint ordering is defined by the given joint names),
   * also getting multiplier and offset values for any mimic joint mapped joints.
   * \param name The name of the joint in JntArray / the KDL chain
   * \param multiplier The multiplier from any mimic joints
   * \param offset The offset from any mimic joints
   * \returns the index of the joint_name to get values from in joint_names
   */
  static size_t find_source(const std::vector<std::string> & joint_names,
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
};

//  JointMap(const std::vector<std::string>& input_names) {
//    if (joint_names.size() == 0) {
//      // Normal behaviour assumes at least one valid value will be given during copy_values_to_jnts
//    }
//
//    // Calculate jnt_joint_names
//    jnt_joint_names.reserve(chain.getNrOfJoints());
//    int joint_count = 0;
//    for (int i = 0; i < chain.getNrOfSegments(); ++i) {
//      const auto& segment = chain.getSegment(i);
//      const auto& joint = segment.getJoint();
//
//      // Skip fixed joints -- I am assuming the JntArray includes only segments with joints attached
//      if (joint.getType() != KDL::Joint::None) {
//        joint_names.push_back(joint.getName());
//        joint_count++;
//      }
//    }
//
//    sources.reserve(jnt_joint_names.size());
//    multipliers.reserve(jnt_joint_names.size());
//    offsets.reserve(jnt_joint_names.size());
//
//    for (auto & name : jnt_joint_names) {
//      double multiplier = 1.0;
//      double offset = 0.0;
//
//      auto source = find_source(joint_names, mimic_joints, name, multiplier, offset);
//
//      sources.emplace_back(source);
//      multipliers.emplace_back(multiplier);
//      offsets.emplace_back(offset);
//    }
//
//    urdf_model.getJoint("joe").
//  }


} // arm_kinematics

#endif //ARM_KINEMATICS_JOINT_MAP_HPP
