//
// Created by Bailey Chessum on 15/12/2025.
//

#ifndef ARM_KINEMATICS_KINEMATICS_HPP
#define ARM_KINEMATICS_KINEMATICS_HPP
#include "plugin_loader.hpp"
#include "collision/collision_manager.hpp"

namespace arm_kinematics {

/// Example construction of various objects from arm_kinematics
struct Kinematics final {
  using Optional = std::optional<Kinematics>;

  struct Forward final {
    ForwardKinematicsPlugin::SharedPtr plugin = nullptr;

    explicit Forward(PluginLoader& plugin_loader)
      : plugin(plugin_loader.make_fk())
    {
    }
  };

  struct Collision final {
    CollisionManager manager{};

    explicit Collision(tl::expected<CollisionManager, const char *> maybe_collision_manager) {
      if (!maybe_collision_manager.has_value()) {
        RCLCPP_FATAL(rclcpp::get_logger("arm_kinematics"), "%s", maybe_collision_manager.error());
        throw std::runtime_error(maybe_collision_manager.error());
      }

      manager = maybe_collision_manager.value();

      // TODO: Make this behaviour configurable, in case the zero pose IS invalid
      manager.allow_current_colliding_pairs();
    }

    explicit Collision(
      PluginLoader& plugin_loader,
      const ForwardKinematicsPlugin::SharedPtr & fk)
        : Collision(make_collision_manager(plugin_loader, fk))
    {
    }
  };

  struct Inverse final {
    InverseKinematicsPlugin::SharedPtr plugin = nullptr;

    explicit Inverse(PluginLoader& plugin_loader)
      : plugin(plugin_loader.make_ik())
    {
    }
  };

  PluginLoader plugin_loader{};

  Forward forward;
  Collision collision;
  Inverse inverse;

  explicit Kinematics(PluginLoader plugin_loader)
  : plugin_loader(std::move(plugin_loader)),
    forward(plugin_loader),
    collision(plugin_loader, forward.plugin), //< TODO: Make make_collision_manager() constexpr and use it here :/
    inverse(plugin_loader)
  {
  }

  explicit Kinematics(PluginLoader::PluginLoaderNodeInterfaces node, std::string robot_description)
    : Kinematics(PluginLoader(std::move(node), std::move(robot_description)))
  {
  }
};

} // arm_kinematics

#endif //ARM_KINEMATICS_KINEMATICS_HPP
