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
 * Helper class to map between the parameterized set of joints and the joints required for FK with transmissions applied
 * \see JointMapBuilder
 */
class ARM_KINEMATICS_PUBLIC JointMap {
public:
  JointMap() = default;
  JointMap(const std::vector<std::string>& input_names,
           const std::vector<std::string>& output_names,
           const std::map<std::string, std::shared_ptr<urdf::JointMimic>> & mimic_joints = {});

  JointMap(const size_t input_count, const size_t output_count) : input_count(input_count), output_count(output_count) {
    sources.resize(output_count, 0);
    multipliers.resize(output_count, 1.0);
    offsets.resize(output_count, 0.0);
  }

  static JointMap identity(const size_t element_count) {
    JointMap jm{element_count, element_count};
    for (size_t i = 0; i < element_count; ++i) {
      jm.sources[i] = i;
    }
    return jm;
  }

  /**
   * \brief Maps all the given input joint positions to output joint positions based on transmissions and mimic joints.
   * \note The values in inputs and outputs should correspond to input_names and output_names provided in the class
   * constructor respectively.
   *
   * \param[in] inputs The position values for each joint defined by input_names in the constructor
   * \param[out] outputs The position values for each joint defined by output_names in the constructor.
   *
   * \warning inputs and outputs must be pre-allocated to the correct size!
   * \warning inputs and outputs must not point to the same memory, or be any of the class's internal vectors.
   */
  void map(const std::vector<double> & inputs, std::vector<double> & outputs) const;

  /**
   * \brief Same as map, but outputs to a KDL::JntArray. Maps all the given input joint positions,
   * to joint positions in outputs based on transmissions and mimic joints.
   * \see JointMap::map
   * \note The values in inputs and outputs should correspond to input_names and output_names provided in the class
   * constructor respectively.
   *
   * \param[in] inputs The position values for each joint defined by input_names in the constructor
   * \param[out] outputs The position values for each joint defined by output_names in the constructor.
   *
   * \warning inputs and outputs must be pre-allocated to the correct size!
   * \warning inputs and outputs must not point to the same memory, or be any of the class's internal vectors.
   */
  void map(const std::vector<double> & inputs, KDL::JntArray & jnts) const;

  /// output_count elements, the index of the value in inputs to use to calculate the value for each output.
  std::vector<size_t> sources{};
  /// output_count elements, the values to multiply each input by when calculating the value for each output.
  std::vector<double> multipliers{};
  /// output_count elements, the values to add when calculating the value for each output.
  std::vector<double> offsets{};

  /// The number of elements in inputs and input_names.
  const size_t input_count = 0;
  /// The number of elements in outputs and output_names.
  const size_t output_count = 0;

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
