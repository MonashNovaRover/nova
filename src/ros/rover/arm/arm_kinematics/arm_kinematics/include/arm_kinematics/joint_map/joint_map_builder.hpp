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

class ARM_KINEMATICS_PUBLIC JointMapBuilder {
public:
  virtual ~JointMapBuilder() = default;

  [[nodiscard]] virtual JointMap build(
    const std::vector<std::string> & input_names,
    const std::vector<std::string> & output_names) const = 0;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP
