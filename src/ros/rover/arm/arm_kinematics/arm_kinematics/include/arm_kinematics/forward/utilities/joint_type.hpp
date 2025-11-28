//
// Created by Bailey Chessum on 25/11/2025.
//

#ifndef ARM_KINEMATICS_JOINT_TYPE_HPP
#define ARM_KINEMATICS_JOINT_TYPE_HPP

#include <optional>
#include <urdf_model/joint.h>
#include <arm_kinematics/visibility_control.h>

namespace arm_kinematics {

enum class ARM_KINEMATICS_PUBLIC JointType {
  REVOLUTE,
  PRISMATIC,
  CONTINUOUS
};

/**
 * Gets the equivalent ComputeJointTree type for a URDF joint
 * \param joint The joint
 * \return std::nullopt, or a joint type
 */
inline std::optional<JointType> get_type(const urdf::JointConstSharedPtr & joint) {
  switch (joint->type) {
    case urdf::Joint::REVOLUTE:
      return JointType::REVOLUTE;
    case urdf::Joint::PRISMATIC:
      return JointType::PRISMATIC;
    case urdf::Joint::CONTINUOUS:
      return JointType::CONTINUOUS;
    default:
      return std::nullopt;
  }
}

/**
 * Gets the equivalent ComputeJointTree type for a URDF joint
 * \param joint The joint
 * \return std::nullopt, or a joint type
 */
inline std::optional<JointType> get_type(const urdf::Joint & joint) {
  switch (joint.type) {
    case urdf::Joint::REVOLUTE:
      return JointType::REVOLUTE;
    case urdf::Joint::PRISMATIC:
      return JointType::PRISMATIC;
    case urdf::Joint::CONTINUOUS:
      return JointType::CONTINUOUS;
    default:
      return std::nullopt;
  }
}

}

#endif //ARM_KINEMATICS_JOINT_TYPE_HPP