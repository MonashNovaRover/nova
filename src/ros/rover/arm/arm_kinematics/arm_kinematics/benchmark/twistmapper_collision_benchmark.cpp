#include <benchmark/benchmark.h>

#include <rclcpp/rclcpp.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "arm_kinematics/collision/collision_config.hpp"
#include "arm_kinematics/collision/collision_manager.hpp"
#include "arm_kinematics/collision/collision_utilities.hpp"
#include "arm_kinematics/forward/frame_definitions.hpp"
#include "arm_kinematics/joint_map/state_interface_definition.hpp"
#include "arm_kinematics/plugin_loader.hpp"
#include "arm_kinematics/utilities/interface_id.hpp"

namespace {

constexpr const char * kBenchmarkDataWorkspaceRelativePath = "benchmark/data/taipan_twistmapper_benchmark.urdf";
constexpr const char * kBenchmarkDataInstallRelativePath = "share/arm_kinematics/benchmark/taipan_twistmapper_benchmark.urdf";
constexpr const char * kBenchmarkDataEnvVar = "ARM_KINEMATICS_BENCHMARK_DATA_DIR";
constexpr const char * kBaseLinkName = "arm_kinematics_origin";
constexpr const char * kEndEffectorLinkName = "endeffector_kinematics";
constexpr const char * kFallbackFrameId = "endeffector_kinematics";
constexpr double kSelfIntersectionMaxStepSize = 0.05;

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

struct RosInitGuard {
  RosInitGuard()
  {
    if (!rclcpp::ok()) {
      const int argc = 0;
      rclcpp::init(argc, nullptr);
    }
  }

  ~RosInitGuard()
  {
    if (rclcpp::ok()) {
      rclcpp::shutdown();
    }
  }
};

const RosInitGuard & ros_guard()
{
  static const RosInitGuard guard;
  return guard;
}

std::shared_ptr<rclcpp::Node> make_benchmark_node(const std::string & name)
{
  (void)ros_guard();

  auto node = std::make_shared<rclcpp::Node>(
    name,
    rclcpp::NodeOptions().arguments({"--ros-args", "--log-level", "error"}));

  node->declare_parameter("base_link_name", std::string{kBaseLinkName});
  node->declare_parameter(
    "kinematics.forward_kinematics_plugin",
    std::string{"arm_kinematics/DefaultForwardKinematicsPlugin"});
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
  const std::vector<std::string> & joint_names,
  const std::string & frame_name)
{
  std::vector<arm_kinematics::NamedStateInterfaceDefinition> named_inputs;
  named_inputs.reserve(joint_names.size());
  for (const auto & joint_name : joint_names) {
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

struct TwistmapperCollisionRuntime {
  std::shared_ptr<rclcpp::Node> node;
  arm_kinematics::PluginLoader loader;
  arm_kinematics::ForwardKinematicsPlugin::SharedPtr fk;
  arm_kinematics::CollisionManager collision_manager;
  std::vector<double> current_joint_state_values;
  std::vector<double> predicted_joint_positions;
  std::vector<double> joint_values_scratch;
  std::vector<std::pair<std::size_t, std::size_t>> colliding_pairs_scratch;

  explicit TwistmapperCollisionRuntime(const std::string & node_name)
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
};

}  // namespace

static void BM_TwistmapperConfigureCollisionStack(benchmark::State & state)
{
  const auto urdf = load_taipan_urdf();
  const auto urdf_path = benchmark_data_path();

  for (auto _ : state) {
    auto node = make_benchmark_node("bm_twistmapper_configure");
    arm_kinematics::PluginLoader loader(*node, urdf);

    auto fk = loader.make_fk();
    if (!fk) {
      state.SkipWithError("Failed to create FK plugin.");
      break;
    }

    const auto collision_config = arm_kinematics::read_collision_config(node->get_node_parameters_interface());
    auto collision_result = arm_kinematics::make_collision_manager(loader, fk, kJointNames, collision_config);
    if (!collision_result) {
      state.SkipWithError(collision_result.error().format().c_str());
      break;
    }

    auto ee_tree = make_single_frame_tree(*fk, kJointNames, kEndEffectorLinkName);
    auto twist_tree = make_single_frame_tree(*fk, kJointNames, kFallbackFrameId);

    benchmark::DoNotOptimize(collision_result->parent_link_names());
    benchmark::DoNotOptimize(ee_tree);
    benchmark::DoNotOptimize(twist_tree);
  }

  state.SetLabel(urdf_path);
}
BENCHMARK(BM_TwistmapperConfigureCollisionStack);

static void BM_TwistmapperCheckPathCollision(benchmark::State & state)
{
  TwistmapperCollisionRuntime runtime("bm_twistmapper_runtime_path");

  for (auto _ : state) {
    auto collision_result = arm_kinematics::check_path_collision(
      runtime.collision_manager,
      runtime.current_joint_state_values,
      runtime.predicted_joint_positions,
      kSelfIntersectionMaxStepSize,
      runtime.joint_values_scratch,
      runtime.colliding_pairs_scratch);
    benchmark::DoNotOptimize(collision_result);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_TwistmapperCheckPathCollision);

static void BM_TwistmapperUpdateAndCollideSingleState(benchmark::State & state)
{
  TwistmapperCollisionRuntime runtime("bm_twistmapper_runtime_single_state");

  for (auto _ : state) {
    runtime.collision_manager.update_poses(runtime.predicted_joint_positions);
    const bool collided = runtime.collision_manager.collide(runtime.colliding_pairs_scratch);
    benchmark::DoNotOptimize(collided);
    benchmark::DoNotOptimize(runtime.colliding_pairs_scratch);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_TwistmapperUpdateAndCollideSingleState);
