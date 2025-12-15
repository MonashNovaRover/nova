//
// Created by nova on 11/16/25.
//

#ifndef ARM_KINEMATICS_KINEMATICS_PLUGIN_SPAWNER_HPP
#define ARM_KINEMATICS_KINEMATICS_PLUGIN_SPAWNER_HPP

#include <pluginlib/class_loader.hpp>

#include "arm_kinematics/forward/forward_kinematics_plugin.hpp"
#include "arm_kinematics/inverse/inverse_kinematics_plugin.hpp"
#include "arm_kinematics/visibility_control.h"
#include "arm_kinematics/collision/discrete_collision_plugin.hpp"

namespace arm_kinematics {

/**
 * Helper struct to load common params for plugins, hold class loaders to load plugin instances, then automatically
 * initialize plugin instances with the shared params.
 *
 * You don't need to use this class. If you want, you can manually create ClassLoaders and initialize the plugins
 * yourself.
 */
class ARM_KINEMATICS_PUBLIC PluginLoader {
public:
  using PluginLoaderNodeInterfaces = KinematicsBase::KinematicsNodeInterfaces;

  /**
   * Constructor
   * \param node Node to pass to created plugins, to get read default plugin types from parameters from, and (if needed)
   * to read KinematicsParams from.
   * \param robot_description The URDF string to provide to any spawned kinematics plugins. Lifetime must exceed this.
   */
  explicit PluginLoader(PluginLoaderNodeInterfaces node, std::string robot_description);

  /// Make an FK plugin using the plugin name defined in the `kinematics.forward_kinematics_plugin` parameter.
  ForwardKinematicsPlugin::SharedPtr make_fk();
  /// Make an FK plugin, manually specifying the plugin name.
  ForwardKinematicsPlugin::SharedPtr make_fk(const std::string & name);

  /// Make an IK plugin using the plugin name defined in the `kinematics.inverse_kinematics_plugin` parameter.
  InverseKinematicsPlugin::SharedPtr make_ik();
  /// Make an IK plugin, manually specifying the plugin name.
  InverseKinematicsPlugin::SharedPtr make_ik(const std::string & name);

  /// Make a collision plugin using the plugin name defined in the `kinematics.collision_plugin` parameter.
  DiscreteCollisionPlugin::SharedPtr make_collision(
    const std::vector<std::reference_wrapper<const urdf::Collision>> & collider_geometries,
    AllowedCollisionMatrix acm);
  /// Make a collision plugin, manually specifying the plugin name.
  DiscreteCollisionPlugin::SharedPtr make_collision(
    const std::string & name,
    const std::vector<std::reference_wrapper<const urdf::Collision>> & collider_geometries,
    AllowedCollisionMatrix acm);

  /// Used to return a FK tree and collision plugin at the same time. Get each with structured bindings.
  struct MakeCollisionResult {
    ForwardKinematicsPlugin::Tree::SharedPtr fk_tree;
    DiscreteCollisionPlugin::SharedPtr collision;
  };

  /// Make a collision plugin with associated ForwardKinematicsPlugin::Tree, using the plugin name defined in the
  /// `kinematics.collision_plugin` parameter
  MakeCollisionResult make_collision(
    const std::vector<std::string> & joint_names,
    const ForwardKinematicsPlugin::SharedPtr & fk);
  /// Make a collision plugin with associated ForwardKinematicsPlugin::Tree, manually specifying the plugin name.
  MakeCollisionResult make_collision(
    const std::string & name,
    const std::vector<std::string> & joint_names,
    const ForwardKinematicsPlugin::SharedPtr & fk);

  /// Lazy gets kinematics params
  [[nodiscard]] const KinematicsParams::SharedPtr & get_kinematics_params() noexcept;

  /// Lazy loads a ClassLoader for ForwardKinematicsPlugin instances
  [[nodiscard]] pluginlib::ClassLoader<ForwardKinematicsPlugin> & get_fk_loader() const noexcept;

  /// Lazy loads a ClassLoader for InverseKinematicsPlugin instances
  [[nodiscard]] pluginlib::ClassLoader<InverseKinematicsPlugin> & get_ik_loader() const noexcept;

  /// Lazy loads a ClassLoader for CollisionPlugin instances
  [[nodiscard]] pluginlib::ClassLoader<DiscreteCollisionPlugin> & get_collision_loader() const noexcept;

private:
  PluginLoaderNodeInterfaces node_;
  RobotModel::UniquePtr robot_model_ = nullptr;

  /// Lazily loaded params for kinematics plugins
  KinematicsParams::SharedPtr kinematics_params_;

  // Lazily created class loaders
  mutable std::unique_ptr<pluginlib::ClassLoader<ForwardKinematicsPlugin>> fk_loader_;
  mutable std::unique_ptr<pluginlib::ClassLoader<InverseKinematicsPlugin>> ik_loader_;
  mutable std::unique_ptr<pluginlib::ClassLoader<DiscreteCollisionPlugin>> collision_loader_;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_KINEMATICS_PLUGIN_SPAWNER_HPP