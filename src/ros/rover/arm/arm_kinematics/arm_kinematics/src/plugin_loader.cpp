//
// Created by nova on 11/16/25.
//

#include <arm_kinematics/plugin_loader.hpp>

#include "arm_kinematics/utilities/param_reader.hpp"

namespace arm_kinematics {
PluginLoader::PluginLoader(
  PluginLoaderNodeInterfaces node,
  const std::string & robot_description)
: node_(std::move(node)),
  robot_description_(robot_description)
{
}

ForwardKinematicsPlugin::SharedPtr PluginLoader::make_fk(const std::string & name) {
  auto plugin = get_fk_loader().createSharedInstance(name);
  if (plugin->initialize(node_, get_kinematics_params()))
    return plugin;

  auto logger = node_.get_node_logging_interface()->get_logger();
  RCLCPP_ERROR(logger, "Failed to initialize FK plugin \"%s\".", name.c_str());
  return nullptr;
}

ForwardKinematicsPlugin::SharedPtr PluginLoader::make_fk() {
  const ParamReader params(node_.get_node_parameters_interface());
  const auto name = params.get_or<std::string>(
    "kinematics.forward_kinematics_plugin",
    "arm_kinematics/EigenForwardKinematicsPlugin");

  return make_fk(name);
}

InverseKinematicsPlugin::SharedPtr PluginLoader::make_ik(const std::string & name) {
  auto plugin = get_ik_loader().createSharedInstance(name);
  if (plugin->initialize(node_, get_kinematics_params()))
    return plugin;

  auto logger = node_.get_node_logging_interface()->get_logger();
  RCLCPP_ERROR(logger, "Failed to initialize IK plugin \"%s\".", name.c_str());
  return nullptr;
}


InverseKinematicsPlugin::SharedPtr PluginLoader::make_ik() {
  const ParamReader params(node_.get_node_parameters_interface());
  const auto name = params.get_or<std::string>("kinematics.inverse_kinematics_plugin", "");

  if (name.empty()) {
    auto logger = node_.get_node_logging_interface()->get_logger();
    RCLCPP_ERROR(logger, "Failed to make IK plugin instance -- kinematics.inverse_kinematics_plugin parameter was left "
                         "unspecified!\nPlease set a value for kinematics.inverse_kinematics_plugin, as there is no "
                         "default IK implementation.");
    return nullptr;
  }

  return make_ik(name);
}

CollisionPlugin::SharedPtr PluginLoader::make_collision(
  const std::vector<urdf::Collision> & collider_geometries,
  AllowedCollisionMatrix acm)
{
  const ParamReader params(node_.get_node_parameters_interface());
  const auto name = params.get_or<std::string>(
    "kinematics.collision_plugin",
    "arm_kinematics/FclCollisionPlugin");

  return make_collision(name, collider_geometries, std::move(acm));
}

CollisionPlugin::SharedPtr PluginLoader::make_collision(
  const std::string & name,
  const std::vector<urdf::Collision> & collider_geometries,
  AllowedCollisionMatrix acm)
{
  auto plugin = get_collision_loader().createSharedInstance(name);
  if (plugin->initialize(node_, collider_geometries, std::move(acm)))
    return plugin;

  auto logger = node_.get_node_logging_interface()->get_logger();
  RCLCPP_ERROR(logger, "Failed to initialize collision plugin \"%s\".", name.c_str());
  return nullptr;
}

const KinematicsParams::SharedPtr & PluginLoader::get_kinematics_params() noexcept {
  if (!kinematics_params_) {
    auto params = node_.get<rclcpp::node_interfaces::NodeParametersInterface>();
    kinematics_params_ = std::make_shared<KinematicsParams>(params, robot_description_);
  }

  return kinematics_params_;
}

pluginlib::ClassLoader<ForwardKinematicsPlugin> & PluginLoader::get_fk_loader() const noexcept {
  if (!fk_loader_)
    fk_loader_ = std::make_unique<pluginlib::ClassLoader<ForwardKinematicsPlugin>>(
      "arm_kinematics", "arm_kinematics::ForwardKinematicsPlugin");

  return *fk_loader_;
}

pluginlib::ClassLoader<InverseKinematicsPlugin> & PluginLoader::get_ik_loader() const noexcept {
  if (!ik_loader_)
    ik_loader_ = std::make_unique<pluginlib::ClassLoader<InverseKinematicsPlugin>>(
      "arm_kinematics", "arm_kinematics::InverseKinematicsPlugin");

  return *ik_loader_;
}

pluginlib::ClassLoader<CollisionPlugin> & PluginLoader::get_collision_loader() const noexcept {
  if (!collision_loader_)
    collision_loader_ = std::make_unique<pluginlib::ClassLoader<CollisionPlugin>>(
      "arm_kinematics", "arm_kinematics::CollisionPlugin");

  return *collision_loader_;
}

} // arm_kinematics