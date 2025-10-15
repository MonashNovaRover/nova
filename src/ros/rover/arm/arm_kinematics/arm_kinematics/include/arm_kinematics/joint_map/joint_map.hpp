//
// Created by Bailey Chessum on 14/10/2025.
//

#ifndef ARM_KINEMATICS_JOINT_MAP_HPP
#define ARM_KINEMATICS_JOINT_MAP_HPP

#include <arm_kinematics/visibility_control.h>
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
class ARM_KINEMATICS_PUBLIC JointMap {
public:
  JointMap() = default;
  JointMap(const std::vector<std::string>& input_names,
           const std::vector<std::string>& output_names,
           const std::map<std::string, std::shared_ptr<urdf::JointMimic>> & mimic_joints = {});

  void copy_values_to_jnts(const std::vector<double> & inputs, KDL::JntArray & jnts) const;

  void copy_values(const std::vector<double> & inputs, std::vector<double> & outputs) const;

  std::vector<size_t> sources{};
  std::vector<double> multipliers{};
  std::vector<double> offsets{};

  size_t input_count = 0;
  size_t output_count = 0;

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
                            const std::string & name, double & multiplier, double & offset);
};

} // arm_kinematics

#endif //ARM_KINEMATICS_JOINT_MAP_HPP
