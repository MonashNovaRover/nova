//
// Created by nova on 11/30/25.
//

#ifndef ARM_KINEMATICS_COLLIDER_DEFINITIONS_HPP
#define ARM_KINEMATICS_COLLIDER_DEFINITIONS_HPP

#include <vector>
#include <arm_kinematics/forward/frame_definitions.hpp>
#include "allowed_collision_matrix.hpp"
#include <urdf/urdf/model.h>

namespace arm_kinematics {

/**
 * Use this to extract data needed to make CollisionPlugin instances by providing a URDF model to the constructor.
 */
struct ARM_KINEMATICS_PUBLIC ColliderDefinitions {
  std::vector<std::reference_wrapper<const urdf::Collision>> colliders{};
  FrameDefinitions frames{};
  AllowedCollisionMatrix acm{};

  explicit ColliderDefinitions(const urdf::Model & urdf_model);
};

} // arm_kinematics

#endif //ARM_KINEMATICS_COLLIDER_DEFINITIONS_HPP