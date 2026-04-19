//
// Created by Codex on 19/04/2026.
//

#ifndef ARM_KINEMATICS_COLLISION_CONFIG_HPP
#define ARM_KINEMATICS_COLLISION_CONFIG_HPP

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

struct ARM_KINEMATICS_PUBLIC CollisionConfig {
  bool generate_from_default_pose = true;
  std::unordered_map<std::string, double> default_pose_overrides{};
  std::vector<std::pair<std::string, std::string>> allowed_pairs{};
};

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_COLLISION_CONFIG_HPP
