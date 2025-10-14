//
// Created by Bailey Chessum on 14/10/2025.
//

#ifndef ARM_KINEMATICS_MIMIC_JOINT_MAP_HPP
#define ARM_KINEMATICS_MIMIC_JOINT_MAP_HPP

#include <cstddef>
#include <urdf/model.h>
#include <kdl_parser/kdl_parser.hpp>
#include <kdl/jntarray.hpp>
#include <rclcpp/logger.hpp>
#include <rclcpp/logging.hpp>


namespace arm_kinematics {

struct MimicJoint {
  std::string source_name;  // Name of the joint it mimics
  double multiplier;    // <mimic multiplier="...">
  double offset;        // <mimic offset="...">
};

/**
 * Helper class to map between the parameterized set of joints and the KDL chain/JntArray joints.
 */
class MimicJointMap {
public:
  MimicJointMap() = default;
  MimicJointMap(rclcpp::Logger logger,
                std::vector<std::string>& joint_names,
                const urdf::Model& urdf_model,
                KDL::Chain& chain) {

    if (joint_names.size() == 0) {
      // Normal behaviour assumes at least one valid value will be given during copy_values_to_jnts
    }

    // Calculate jnt_joint_names
    jnt_joint_names.reserve(chain.getNrOfJoints());
    int joint_count = 0;
    for (int i = 0; i < chain.getNrOfSegments(); ++i) {
      const auto& segment = chain.getSegment(i);
      const auto& joint = segment.getJoint();

      // Skip fixed joints -- I am assuming the JntArray includes only segments with joints attached
      if (joint.getType() != KDL::Joint::None) {
        joint_names.push_back(joint.getName());
        joint_count++;
      }
    }

    // Ensure our assumption is correct
    assert(joint_count == chain.getNrOfJoints());


    // Find all mimic joints
    // TODO: guard against cyclic mimic joints
    std::map<std::string, std::shared_ptr<urdf::JointMimic>> mimic_joints{};
    for (const auto & [name, joint] : urdf_model.joints_) {
      if (!joint->mimic)
        continue;

      // If for some reason a joint in joint_names is a mimic joint, just pretend it isn't
      auto it = std::find(joint_names.begin(), joint_names.end(), name);
      if (it != joint_names.end()) {
        RCLCPP_WARN(logger,
                    "Joint \"%s\" was included in joint_names, but it is a mimic joint. Are you sure this is ok?"
                    "It will be treated as if it weren't actually a mimic joint.", name.c_str());
        continue;
      }

      if (name == joint->mimic->joint_name) {
        RCLCPP_WARN(logger,
                    "Joint \"%s\" mimics itself.", name.c_str());
        continue;
      }

      mimic_joints[name] = joint->mimic;
    }

    sources.reserve(jnt_joint_names.size());
    multipliers.reserve(jnt_joint_names.size());
    offsets.reserve(jnt_joint_names.size());

    for (auto & name : jnt_joint_names) {
      double multiplier = 1.0;
      double offset = 0.0;

      auto source = find_source(joint_names, mimic_joints, name, multiplier, offset);

      sources.emplace_back(source);
      multipliers.emplace_back(multiplier);
      offsets.emplace_back(offset);
    }




  }


  void copy_values_to_jnts(const std::vector<double> & values, KDL::JntArray & jnts) {
    if (values.size() != given_joint_name_count) {
      // TODO: Throw exception
      return;
    }

  }

  std::vector<size_t> sources{};
  std::vector<double> multipliers{};
  std::vector<double> offsets{};

  size_t given_joint_name_count = 0;

  std::vector<std::string> jnt_joint_names{};

private:
  /**
   * Recursively maps joint names to source value ids (where joint ordering is defined by the given joint names),
   * also getting multiplier and offset values for any mimic joint mapped joints.
   * \param name The name of the joint in JntArray / the KDL chain
   * \param multiplier The multiplier from any mimic joints
   * \param offset The offset from any mimic joints
   * \returns the index of the joint_name to get values from in joint_names
   */
  size_t find_source(std::vector<std::string>& joint_names,
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

} // arm_kinematics

#endif //ARM_KINEMATICS_MIMIC_JOINT_MAP_HPP
