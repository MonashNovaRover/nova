//
// Created by Codex on 19/04/2026.
//

#ifndef ARM_KINEMATICS_COLLISION_BUILD_ERROR_HPP
#define ARM_KINEMATICS_COLLISION_BUILD_ERROR_HPP

#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include "arm_kinematics/forward/forward_kinematics_plugin.hpp"

namespace arm_kinematics {

struct MakeCollisionError {
  struct MakeTreeFailed {
    ForwardKinematicsPlugin::MakeTreeError error;

    [[nodiscard]] std::string format() const;
  };

  struct CollisionPluginInitFailed {
    std::string detail{};

    [[nodiscard]] std::string format() const;
  };

  using Variant = std::variant<MakeTreeFailed, CollisionPluginInitFailed>;
  Variant value;

  template <
    typename T,
    typename Decayed = std::decay_t<T>,
    typename = std::enable_if_t<!std::is_same_v<Decayed, MakeCollisionError>>>
  MakeCollisionError(T && t)
  : value(std::forward<T>(t))
  {
  }

  [[nodiscard]] std::string format() const;
};

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_COLLISION_BUILD_ERROR_HPP
