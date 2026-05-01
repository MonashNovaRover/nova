//
// Created by Bailey Chessum on 11/16/25.
//

#include <arm_kinematics/plugin_loader.hpp>
#include <arm_kinematics/collision/collider_definitions.hpp>
#include <arm_kinematics/joint_map/state_interface_definition.hpp>
#include <arm_kinematics/utilities/param_reader.hpp>
#include <arm_kinematics/utilities/to_eigen.hpp>

#include <stdexcept>
#include <utility>

namespace arm_kinematics {

namespace {

// Helper to convert a vector of joint name strings into the position-interface
// `NamedStateInterfaceDefinition`s expected by the FK plugin's named-overload of make_tree.
std::vector<NamedStateInterfaceDefinition> joint_names_to_position_named_interfaces(
  const std::vector<std::string> & joint_names)
{
  static const InterfaceId k_position{"position"};
  std::vector<NamedStateInterfaceDefinition> result;
  result.reserve(joint_names.size());
  for (const auto & name : joint_names) {
    result.push_back(NamedStateInterfaceDefinition{name, k_position});
  }
  return result;
}

AllowedCollisionMatrix remap_acm(const AllowedCollisionMatrix & acm, const Order<> & order)
{
  std::vector<std::size_t> new_to_old;
  new_to_old.reserve(order.size());

  for (const auto old_index : order) {
    new_to_old.push_back(old_index);
  }

  return acm.remap(span<const std::size_t>(new_to_old.data(), new_to_old.size()));
}

struct CompactedCollisionInputs {
  std::vector<std::reference_wrapper<const urdf::Collision>> colliders;
  FrameDefinitions frames;
  AllowedCollisionMatrix acm;
};

CompactedCollisionInputs compact_supported_colliders(
  const std::vector<std::reference_wrapper<const urdf::Collision>> & colliders,
  const FrameDefinitions & frames,
  const AllowedCollisionMatrix & acm,
  const DiscreteCollisionPlugin & plugin,
  const rclcpp::Logger & logger)
{
  std::vector<std::size_t> kept_indices;
  kept_indices.reserve(colliders.size());

  for (std::size_t i = 0; i < colliders.size(); ++i) {
    if (plugin.supports_geometry(colliders[i])) {
      kept_indices.push_back(i);
      continue;
    }

    const auto & link_name = i < frames.parent_link_names.size() ? frames.parent_link_names[i] : std::string{};
    if (link_name.empty()) {
      RCLCPP_WARN(logger, "Excluding unsupported collider %zu from collision construction.", i);
    } else {
      RCLCPP_WARN(
        logger,
        "Excluding unsupported collider %zu on link \"%s\" from collision construction.",
        i,
        link_name.c_str());
    }
  }

  if (kept_indices.size() == colliders.size()) {
    return CompactedCollisionInputs{
      colliders,
      frames,
      AllowedCollisionMatrix(acm, acm.capacity),
    };
  }

  std::vector<std::reference_wrapper<const urdf::Collision>> compacted_colliders;
  compacted_colliders.reserve(kept_indices.size());
  std::vector<std::string> compacted_parent_link_names;
  compacted_parent_link_names.reserve(kept_indices.size());
  Isometry3dVector compacted_origins;
  compacted_origins.reserve(kept_indices.size());

  for (const auto old_index : kept_indices) {
    compacted_colliders.push_back(colliders[old_index]);
    compacted_parent_link_names.push_back(frames.parent_link_names[old_index]);
    compacted_origins.push_back(frames.origins[old_index]);
  }

  return CompactedCollisionInputs{
    std::move(compacted_colliders),
    FrameDefinitions(std::move(compacted_parent_link_names), std::move(compacted_origins)),
    acm.remap(span<const std::size_t>(kept_indices.data(), kept_indices.size())),
  };
}

}  // namespace

PluginLoader::PluginLoader(
  PluginLoaderNodeInterfaces node,
  std::string robot_description)
: node_(std::move(node)),
  robot_model_(std::make_unique<RobotModel>(std::move(robot_description)))
{
}

ForwardKinematicsPlugin::SharedPtr PluginLoader::make_fk(const std::string & name) {
  auto plugin = get_fk_loader().createSharedInstance(name);
  if (plugin->initialize(node_, *robot_model_, get_kinematics_params()))
    return plugin;

  auto logger = node_.get_node_logging_interface()->get_logger();
  RCLCPP_ERROR(logger, "Failed to initialize FK plugin \"%s\".", name.c_str());
  return nullptr;
}

ForwardKinematicsPlugin::SharedPtr PluginLoader::make_fk() {
  const ParamReader params(node_.get_node_parameters_interface());
  const auto name = params.get<std::string>(
    "kinematics.forward_kinematics_plugin",
    "arm_kinematics/DefaultForwardKinematicsPlugin");

  return make_fk(name);
}

InverseKinematicsPlugin::SharedPtr PluginLoader::make_ik(const std::string & name) {
  auto plugin = get_ik_loader().createSharedInstance(name);
  if (plugin->initialize(node_, *robot_model_, get_kinematics_params()))
    return plugin;

  auto logger = node_.get_node_logging_interface()->get_logger();
  RCLCPP_ERROR(logger, "Failed to initialize IK plugin \"%s\".", name.c_str());
  return nullptr;
}

InverseKinematicsPlugin::SharedPtr PluginLoader::make_ik() {
  const ParamReader params(node_.get_node_parameters_interface());
  const auto name = params.get<std::string>("kinematics.inverse_kinematics_plugin", "");

  if (name.empty()) {
    auto logger = node_.get_node_logging_interface()->get_logger();
    RCLCPP_ERROR(logger, "Failed to make IK plugin instance -- kinematics.inverse_kinematics_plugin parameter was left "
                         "unspecified!\nPlease set a value for kinematics.inverse_kinematics_plugin, as there is no "
                         "default IK implementation.");
    return nullptr;
  }

  return make_ik(name);
}

DiscreteCollisionPlugin::SharedPtr PluginLoader::make_collision(
  const std::vector<std::reference_wrapper<const urdf::Collision>> & collider_geometries,
  AllowedCollisionMatrix acm)
{
  const ParamReader params(node_.get_node_parameters_interface());
  const auto name = params.get<std::string>(
    "kinematics.collision_plugin",
    "arm_kinematics/FclCollisionPlugin");

  return make_collision(name, collider_geometries, std::move(acm));
}

DiscreteCollisionPlugin::SharedPtr PluginLoader::make_collision(
  const std::string & name,
  const std::vector<std::reference_wrapper<const urdf::Collision>> & collider_geometries,
  AllowedCollisionMatrix acm)
{
  auto plugin = get_collision_loader().createSharedInstance(name);
  if (plugin->initialize(node_, collider_geometries, std::move(acm)))
    return plugin;

  auto logger = node_.get_node_logging_interface()->get_logger();
  RCLCPP_ERROR(logger, "Failed to initialize collision plugin \"%s\".", name.c_str());
  return nullptr;
}

tl::expected<PluginLoader::MakeCollisionResult, MakeCollisionError> PluginLoader::make_collision(
  const std::vector<std::string> & joint_names,
  const ForwardKinematicsPlugin::SharedPtr & fk)
{
  return make_collision(joint_names, fk, {});
}

tl::expected<PluginLoader::MakeCollisionResult, MakeCollisionError> PluginLoader::make_collision(
  const std::vector<std::string> & joint_names,
  const ForwardKinematicsPlugin::SharedPtr & fk,
  const span<const std::string> ignored_links)
{
  const auto & urdf_model = robot_model_->get_urdf_model();
  const auto & base_link_name = get_kinematics_params()->base_link_name;
  auto plugin = get_collision_loader().createSharedInstance(
    ParamReader(node_.get_node_parameters_interface()).get<std::string>(
      "kinematics.collision_plugin",
      "arm_kinematics/FclCollisionPlugin"));
  auto [colliders, frames, acm] = ColliderDefinitions(urdf_model, ignored_links);
  auto compacted = compact_supported_colliders(
    colliders,
    frames,
    acm,
    *plugin,
    node_.get_node_logging_interface()->get_logger().get_child("collision"));
  auto parent_link_names = compacted.frames.parent_link_names;
  const auto named_inputs = joint_names_to_position_named_interfaces(joint_names);
  auto tree_result = fk->make_tree(
    span<const NamedStateInterfaceDefinition>(named_inputs.data(), named_inputs.size()),
    base_link_name,
    compacted.frames);
  if (!tree_result) {
    return tl::unexpected(MakeCollisionError::MakeTreeFailed{std::move(tree_result.error())});
  }
  auto [tree, order] = std::move(tree_result.value());

  if (!plugin->initialize(node_, order.reorder(std::move(compacted.colliders)), remap_acm(compacted.acm, order))) {
    auto logger = node_.get_node_logging_interface()->get_logger();
    RCLCPP_ERROR(logger, "Failed to initialize collision plugin from configured parameters.");
    return tl::unexpected(MakeCollisionError::CollisionPluginInitFailed{
      "PluginLoader::make_collision: failed to initialize collision plugin from configured parameters.",
    });
  }

  return MakeCollisionResult{
    std::move(tree),
    std::move(plugin),
    order.reorder(std::move(parent_link_names))
  };
}

tl::expected<PluginLoader::MakeCollisionResult, MakeCollisionError> PluginLoader::make_collision(
  const std::string & name,
  const std::vector<std::string> & joint_names,
  const ForwardKinematicsPlugin::SharedPtr & fk)
{
  return make_collision(name, joint_names, fk, {});
}

tl::expected<PluginLoader::MakeCollisionResult, MakeCollisionError> PluginLoader::make_collision(
  const std::string & name,
  const std::vector<std::string> & joint_names,
  const ForwardKinematicsPlugin::SharedPtr & fk,
  const span<const std::string> ignored_links)
{
  if (!is_valid())
    throw std::logic_error("make_collision(name, joint_names, fk) was called for a default constructed PluginLoader");

  const auto & urdf_model = robot_model_->get_urdf_model();
  const auto & base_link_name = get_kinematics_params()->base_link_name;
  auto [colliders, frames, acm] = ColliderDefinitions(urdf_model, ignored_links);
  auto plugin = get_collision_loader().createSharedInstance(name);
  auto compacted = compact_supported_colliders(
    colliders,
    frames,
    acm,
    *plugin,
    node_.get_node_logging_interface()->get_logger().get_child("collision"));
  auto parent_link_names = compacted.frames.parent_link_names;
  const auto named_inputs = joint_names_to_position_named_interfaces(joint_names);
  auto tree_result = fk->make_tree(
    span<const NamedStateInterfaceDefinition>(named_inputs.data(), named_inputs.size()),
    base_link_name,
    compacted.frames);
  if (!tree_result) {
    return tl::unexpected(MakeCollisionError::MakeTreeFailed{std::move(tree_result.error())});
  }
  auto [tree, order] = std::move(tree_result.value());

  if (!plugin->initialize(node_, order.reorder(std::move(compacted.colliders)), remap_acm(compacted.acm, order))) {
    auto logger = node_.get_node_logging_interface()->get_logger();
    RCLCPP_ERROR(logger, "Failed to initialize collision plugin \"%s\".", name.c_str());
    return tl::unexpected(MakeCollisionError::CollisionPluginInitFailed{
      "PluginLoader::make_collision: failed to initialize collision plugin \"" + name + "\".",
    });
  }

  return MakeCollisionResult{
    std::move(tree),
    std::move(plugin),
    order.reorder(std::move(parent_link_names))
  };
}

const RobotModel & PluginLoader::get_robot_model() const {
  if (!is_valid())
    throw std::logic_error("get_robot_model() was called for a default constructed PluginLoader");

  return *robot_model_;
}

const KinematicsParams::SharedPtr & PluginLoader::get_kinematics_params() {
  if (!is_valid())
    throw std::logic_error("get_kinematics_params() was called for a default constructed PluginLoader");

  if (!kinematics_params_) {
    auto params = node_.get<rclcpp::node_interfaces::NodeParametersInterface>();
    kinematics_params_ = std::make_shared<KinematicsParams>(params);
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

pluginlib::ClassLoader<DiscreteCollisionPlugin> & PluginLoader::get_collision_loader() const noexcept {
  if (!collision_loader_)
    collision_loader_ = std::make_unique<pluginlib::ClassLoader<DiscreteCollisionPlugin>>(
      "arm_kinematics", "arm_kinematics::DiscreteCollisionPlugin");

  return *collision_loader_;
}

bool PluginLoader::is_valid() const noexcept {
  return robot_model_ != nullptr;
}

} // arm_kinematics
