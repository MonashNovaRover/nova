//
// Created by Codex on 21/03/2026.
//

#ifndef ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP
#define ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP

#include <string>
#include <vector>

#include "arm_kinematics/joint_map/joint_map.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * Builder interface for constructing JointMap instances.
 *
 * A builder is responsible for understanding how one named joint space relates to another and for producing the
 * cheapest valid runtime JointMap implementation for that relationship.
 *
 * The default shared implementation is DefaultJointMapBuilder, but FK plugins may return specialized builders from
 * ForwardKinematicsPlugin::get_joint_map_builder() to express backend-specific mapping policies.
 */
class ARM_KINEMATICS_PUBLIC JointMapBuilder {
public:
  virtual ~JointMapBuilder() = default;

  /**
   * Constructs a JointMap that maps inputs with input_names to outputs with output_names.
   *
   * \warning Not real-time safe.
   */
  [[nodiscard]] virtual JointMap build(
    const std::vector<std::string> & input_names,
    const std::vector<std::string> & output_names) const = 0;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP
