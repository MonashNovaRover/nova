#include "twistmapper_benchmark_support.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <utility>

#include <rclcpp/rclcpp/init_options.hpp>

#include "arm_kinematics/collision/collision_config.hpp"
#include "arm_kinematics/collision/collision_utilities.hpp"
#include "arm_kinematics/joint_map/state_interface_definition.hpp"
#include "arm_kinematics/utilities/interface_id.hpp"

namespace arm_kinematics_benchmark {
namespace {

std::string first_existing_path(const std::vector<std::filesystem::path> & candidates, const std::string & what)
{
  namespace fs = std::filesystem;

  for (const auto & candidate : candidates) {
    if (fs::exists(candidate)) {
      return candidate.lexically_normal().string();
    }
  }

  throw std::runtime_error("Failed to locate " + what + ".");
}

std::optional<std::string> env_or_null(const char * name)
{
  const char * value = std::getenv(name);
  if (value == nullptr || *value == '\0') {
    return std::nullopt;
  }

  return std::string(value);
}

}  // namespace

const std::vector<std::string> kJointNames{
  "j1", "j2", "j3", "j4", "j5", "j6"
};

const std::vector<std::string> kIgnoredLinks{
  "diffbar",
  "left_leg",
  "right_leg",
  "back_left_pivot",
  "front_left_pivot",
  "back_right_pivot",
  "front_right_pivot",
  "back_left_wheel",
  "front_left_wheel",
  "back_right_wheel",
  "front_right_wheel",
  "bl_ankle",
  "fl_ankle",
  "br_ankle",
  "fr_ankle",
  "bl_wheel",
  "fl_wheel",
  "br_wheel",
  "fr_wheel",
};

RosInitGuard::RosInitGuard()
{
  if (!rclcpp::ok()) {
    rclcpp::InitOptions options;
    options.auto_initialize_logging(false);
    options.set_domain_id(222);

    const int argc = 0;
    rclcpp::init(argc, nullptr, options);
  }
}

RosInitGuard::~RosInitGuard()
{
  if (rclcpp::ok()) {
    rclcpp::shutdown();
  }
}

const RosInitGuard & ros_guard()
{
  static const RosInitGuard guard;
  return guard;
}

std::string benchmark_data_path()
{
  namespace fs = std::filesystem;

  if (const auto env_root = env_or_null(kBenchmarkDataEnvVar)) {
    const auto candidate = fs::path(*env_root) / "taipan_twistmapper_benchmark.urdf";
    if (fs::exists(candidate)) {
      return candidate.lexically_normal().string();
    }
  }

  return first_existing_path(
    {
      fs::current_path() / kBenchmarkDataWorkspaceRelativePath,
      fs::current_path() / kBenchmarkDataInstallRelativePath,
      fs::path(ARM_KINEMATICS_SOURCE_DIR) / kBenchmarkDataWorkspaceRelativePath,
      fs::path(ARM_KINEMATICS_INSTALL_DATA_DIR) / "taipan_twistmapper_benchmark.urdf",
    },
    "twistmapper benchmark URDF");
}

std::string load_taipan_urdf()
{
  static const std::string urdf = [] {
    const auto path = benchmark_data_path();
    std::ifstream file(path);
    if (!file) {
      throw std::runtime_error("Failed to open benchmark URDF: " + path);
    }

    return std::string(
      std::istreambuf_iterator<char>(file),
      std::istreambuf_iterator<char>());
  }();
  return urdf;
}

std::shared_ptr<rclcpp::Node> make_benchmark_node(const std::string & name, const bool enable_ik)
{
  (void)ros_guard();

  auto node = std::make_shared<rclcpp::Node>(
    name,
    rclcpp::NodeOptions().arguments({"--ros-args", "--log-level", "error"}));

  node->declare_parameter("base_link_name", std::string{kBaseLinkName});
  node->declare_parameter(
    "kinematics.forward_kinematics_plugin",
    std::string{"arm_kinematics/DefaultForwardKinematicsPlugin"});
  if (enable_ik) {
    node->declare_parameter(
      "kinematics.inverse_kinematics_plugin",
      std::string{"arm_kinematics/BanksiaIKPlugin"});
  }
  node->declare_parameter(
    "kinematics.collision_plugin",
    std::string{"arm_kinematics/FclCollisionPlugin"});
  node->declare_parameter("collision.generate_from_default_pose", true);
  node->declare_parameter("collision.ignored_links", kIgnoredLinks);

  const std::vector<std::string> default_joint_values{
    "chassis_to_left_leg=0.0",
    "chassis_to_right_leg=0.0",
    "blp=0.0",
    "brp=0.0",
    "flp=0.0",
    "frp=0.0",
    "blw=0.0",
    "brw=0.0",
    "flw=0.0",
    "frw=0.0",
  };
  node->declare_parameter("kinematics.default_joint_values", default_joint_values);

  return node;
}

arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr make_single_frame_tree(
  arm_kinematics::ForwardKinematicsPlugin & fk,
  const std::string & frame_name)
{
  std::vector<arm_kinematics::NamedStateInterfaceDefinition> named_inputs;
  named_inputs.reserve(kJointNames.size());
  for (const auto & joint_name : kJointNames) {
    named_inputs.emplace_back(joint_name, arm_kinematics::InterfaceId::Position());
  }

  auto tree_result = fk.make_tree(
    arm_kinematics::span<const arm_kinematics::NamedStateInterfaceDefinition>(
      named_inputs.data(), named_inputs.size()),
    kBaseLinkName,
    arm_kinematics::FrameDefinitions{frame_name});
  if (!tree_result) {
    throw std::runtime_error("Failed to build FK tree for frame " + frame_name + ": " + tree_result.error().format());
  }

  return arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr(std::move(tree_result.value().tree));
}

TwistmapperCollisionRuntime::TwistmapperCollisionRuntime(const std::string & node_name)
: node(make_benchmark_node(node_name)),
  loader(*node, load_taipan_urdf())
{
  fk = loader.make_fk();
  if (!fk) {
    throw std::runtime_error("Failed to create FK plugin.");
  }

  const auto collision_config = arm_kinematics::read_collision_config(node->get_node_parameters_interface());
  auto collision_result = arm_kinematics::make_collision_manager(loader, fk, kJointNames, collision_config);
  if (!collision_result) {
    throw std::runtime_error("Failed to create collision manager: " + collision_result.error().format());
  }
  collision_manager = std::move(*collision_result);

  current_joint_state_values = {0.0, -0.35, 0.4, 0.1, -0.25, 0.2};
  predicted_joint_positions = {0.08, -0.22, 0.55, 0.18, -0.1, 0.28};
  joint_values_scratch.assign(kJointNames.size(), 0.0);

  const auto collider_count = collision_manager.parent_link_names().size();
  colliding_pairs_scratch.reserve(collider_count > 1 ? collider_count * (collider_count - 1) / 2 : 0);
}

TwistmapperUpdateRuntime::TwistmapperUpdateRuntime(const std::string & node_name)
: node(make_benchmark_node(node_name, true)),
  loader(*node, load_taipan_urdf())
{
  fk = loader.make_fk();
  if (!fk) {
    throw std::runtime_error("Failed to create FK plugin.");
  }

  ik = loader.make_ik();
  if (!ik) {
    throw std::runtime_error("Failed to create IK plugin.");
  }

  ee_tree = make_single_frame_tree(*fk, kEndEffectorLinkName);
  twist_frame_tree = make_single_frame_tree(*fk, kFallbackFrameId);

  ee_tree->position_fk(current_joint_state, fk_pose_buffer);
  seed_ee_pose = fk_pose_buffer.front();

  solution_positions.assign(kJointNames.size(), 0.0);
  solution_velocities.assign(kJointNames.size(), 0.0);

  twist_with_rotation << 0.05, 0.0, 0.0,
                         0.1,  0.0, 0.0;

  twist_linear_only   << 0.05, 0.0, 0.0,
                         0.0,  0.0, 0.0;
}

}  // namespace arm_kinematics_benchmark
