//
// Created by Bailey Chessum on 15/10/2025.
//

#include <arm_kinematics/collision/collision_plugin.hpp>

namespace arm_kinematics {

bool CollisionPlugin::initialize(const ForwardKinematicsPlugin::SharedPtr & fk) {
  if (!fk)
    return false;

  fk_ = fk;

  return on_initialize();
}

const ForwardKinematicsPlugin::SharedPtr & CollisionPlugin::get_fk() const noexcept {
  return fk_;
}

} // arm_kinematics