//
// Created by nova on 11/30/25.
//

#ifndef SCIENCE_COLLIDER_DEFINITIONS_HPP
#define SCIENCE_COLLIDER_DEFINITIONS_HPP

#include <vector>
#include <urdf/model.h>
#include <arm_kinematics/forward/frame_definitions.hpp>

namespace arm_kinematics {

/**
 * Use this to extract data needed to make CollisionPlugin instances by providing a URDF model to the constructor.
 */
struct ColliderDefinitions {
  std::vector<std::reference_wrapper<const urdf::Collision>> colliders{};
  FrameDefinitions frames{};
  AllowedCollisionMatrix acm{};

  ColliderDefinitions(const urdf::Model & urdf_model);
};

} // arm_kinematics

#endif //SCIENCE_COLLIDER_DEFINITIONS_HPP