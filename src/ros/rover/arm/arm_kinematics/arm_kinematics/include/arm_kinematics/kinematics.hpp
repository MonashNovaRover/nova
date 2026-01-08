//
// Created by Bailey Chessum on 15/12/2025.
//

#ifndef ARM_KINEMATICS_KINEMATICS_HPP
#define ARM_KINEMATICS_KINEMATICS_HPP
#include "plugin_loader.hpp"
#include "collision/collision_manager.hpp"

namespace arm_kinematics {

struct Kinematics final {
  using Optional = std::optional<Kinematics>;

  struct Forward final {
    ForwardKinematicsPlugin::SharedPtr plugin = nullptr;
  };

  struct Collision final {
    CollisionManager manager{};

    explicit Collision(PluginLoader::MakeCollisionResult result)

    explicit Collision(
      PluginLoader plugin_loader,
      ForwardKinematicsPlugin::SharedPtr fk)
        : manager(plugin_loader, fk) {}
  };

  struct Inverse final {
    InverseKinematicsPlugin::SharedPtr plugin = nullptr;
  };

  PluginLoader plugin_loader{};

  Forward forward;
  CollisionManager collision;
  Inverse inverse;

  explicit Kinematics(PluginLoader plugin_loader)
  : plugin_loader(std::move(plugin_loader)),
    forward(),
    collision(), //< TODO: Make make_collision_manager() constexpr and use it here :/
    inverse()
  {

  }
};

} // arm_kinematics

#endif //ARM_KINEMATICS_KINEMATICS_HPP
