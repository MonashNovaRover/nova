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
#include "arm_kinematics/inverse/inverse_kinematics_plugin.hpp"
#include "arm_kinematics/joint_map/state_interface_definition.hpp"
#include "arm_kinematics/plugin_loader.hpp"
#include "arm_kinematics/utilities/aliases.hpp"
#include "arm_kinematics/utilities/interface_id.hpp"
#include "arm_kinematics/utilities/utilities.hpp"

namespace {

constexpr const char * kBenchmarkDataWorkspaceRelativePath = "data/taipan_twistmapper_benchmark.urdf";
constexpr const char * kBenchmarkDataInstallRelativePath = "share/arm_kinematics_benchmark/benchmark/taipan_twistmapper_benchmark.urdf";
constexpr const char * kBenchmarkDataEnvVar = "ARM_KINEMATICS_BENCHMARK_DATA_DIR";
constexpr const char * kBaseLinkName = "arm_kinematics_origin";
constexpr const char * kEndEffectorLinkName = "endeffector_kinematics";
constexpr const char * kTwistFrameName = "endeffector_kinematics";
constexpr double kVelocityIkTimeStep = 0.01;

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
    "kinematics.inverse_kinematics_plugin",
    std::string{"arm_kinematics/BanksiaIKPlugin"});
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

// Models the full set of objects nova_twistmapper keeps alive across update cycles.
struct TwistmapperUpdateRuntime {
  std::shared_ptr<rclcpp::Node> node;
  arm_kinematics::PluginLoader loader;
  arm_kinematics::ForwardKinematicsPlugin::SharedPtr fk;
  arm_kinematics::InverseKinematicsPlugin::SharedPtr ik;
  arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr ee_tree;
  arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr twist_frame_tree;

  // Reusable per-cycle buffers
  arm_kinematics::Isometry3dVector fk_pose_buffer{1};
  std::vector<double> current_joint_state{0.0, -0.35, 0.4, 0.1, -0.25, 0.2};
  std::vector<double> solution_positions{kJointNames.size(), 0.0};
  std::vector<double> solution_velocities{kJointNames.size(), 0.0};

  // Pre-computed pose at the seed state for velocity IK
  Eigen::Isometry3d seed_ee_pose{Eigen::Isometry3d::Identity()};

  // A representative non-zero twist: 5 cm/s forward + 0.1 rad/s roll
  arm_kinematics::Twistd twist_with_rotation;
  // A pure translation twist: 5 cm/s forward only
  arm_kinematics::Twistd twist_linear_only;

  explicit TwistmapperUpdateRuntime(const std::string & node_name)
  : node(make_benchmark_node(node_name)),
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

    ee_tree = make_single_frame_tree(*fk, kJointNames, kEndEffectorLinkName);
    twist_frame_tree = make_single_frame_tree(*fk, kJointNames, kTwistFrameName);

    // Compute EE pose at seed state so velocity IK has a valid seed pose.
    ee_tree->position_fk(current_joint_state, fk_pose_buffer);
    seed_ee_pose = fk_pose_buffer.front();

    solution_positions.assign(kJointNames.size(), 0.0);
    solution_velocities.assign(kJointNames.size(), 0.0);

    twist_with_rotation << 0.05, 0.0, 0.0,   // linear: 5 cm/s forward
                           0.1,  0.0, 0.0;    // angular: 0.1 rad/s roll

    twist_linear_only   << 0.05, 0.0, 0.0,
                           0.0,  0.0, 0.0;
  }
};

}  // namespace

// ---------------------------------------------------------------------------
// FK benchmarks
// ---------------------------------------------------------------------------

// Models the per-cycle call that resolves the twist frame (resolve_base_twist).
static void BM_TwistmapperPositionFk(benchmark::State & state)
{
  TwistmapperUpdateRuntime runtime("bm_tm_position_fk");

  for (auto _ : state) {
    runtime.twist_frame_tree->position_fk(runtime.current_joint_state, runtime.fk_pose_buffer);
    benchmark::DoNotOptimize(runtime.fk_pose_buffer);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_TwistmapperPositionFk);

// ---------------------------------------------------------------------------
// apply_twist benchmarks
// ---------------------------------------------------------------------------

// Typical motion: non-zero angular component triggers the AngleAxisd path.
static void BM_TwistmapperApplyTwistWithRotation(benchmark::State & state)
{
  TwistmapperUpdateRuntime runtime("bm_tm_apply_twist_rotation");
  constexpr double kDt = 0.01;
  Eigen::Isometry3d result;

  for (auto _ : state) {
    arm_kinematics::apply_twist(runtime.twist_with_rotation, kDt, runtime.seed_ee_pose, result);
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_TwistmapperApplyTwistWithRotation);

// Pure translation: angular.norm() < 1e-8 takes the cheaper path.
static void BM_TwistmapperApplyTwistLinearOnly(benchmark::State & state)
{
  TwistmapperUpdateRuntime runtime("bm_tm_apply_twist_linear");
  constexpr double kDt = 0.01;
  Eigen::Isometry3d result;

  for (auto _ : state) {
    arm_kinematics::apply_twist(runtime.twist_linear_only, kDt, runtime.seed_ee_pose, result);
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_TwistmapperApplyTwistLinearOnly);

// ---------------------------------------------------------------------------
// make_tree benchmark
// ---------------------------------------------------------------------------

// Models the overhead of switching the active twist frame (happens whenever
// the operator changes the control frame at runtime).
static void BM_TwistmapperMakeSingleFrameTree(benchmark::State & state)
{
  TwistmapperUpdateRuntime runtime("bm_tm_make_tree");

  for (auto _ : state) {
    auto tree = make_single_frame_tree(*runtime.fk, kJointNames, kTwistFrameName);
    benchmark::DoNotOptimize(tree);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_TwistmapperMakeSingleFrameTree);

// ---------------------------------------------------------------------------
// IK benchmarks
// ---------------------------------------------------------------------------

// Models update_position_mode: IK is solved every cycle.
static void BM_TwistmapperGetPositionIk(benchmark::State & state)
{
  TwistmapperUpdateRuntime runtime("bm_tm_position_ik");

  // Target pose: apply a small twist to the seed pose so IK has a valid target.
  const Eigen::Isometry3d target_pose =
    arm_kinematics::apply_twist(runtime.twist_with_rotation, 0.01, runtime.seed_ee_pose);

  for (auto _ : state) {
    auto result = runtime.ik->get_position_ik(
      target_pose,
      arm_kinematics::span<const double>(runtime.current_joint_state.data(), runtime.current_joint_state.size()),
      arm_kinematics::span<double>(runtime.solution_positions.data(), runtime.solution_positions.size()));
    benchmark::DoNotOptimize(result);
    benchmark::DoNotOptimize(runtime.solution_positions);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_TwistmapperGetPositionIk);

// Models update_velocity_mode: velocity IK every cycle.
static void BM_TwistmapperGetVelocityIk(benchmark::State & state)
{
  TwistmapperUpdateRuntime runtime("bm_tm_velocity_ik");

  for (auto _ : state) {
    auto result = runtime.ik->get_velocity_ik(
      runtime.twist_with_rotation,
      runtime.seed_ee_pose,
      arm_kinematics::span<const double>(runtime.current_joint_state.data(), runtime.current_joint_state.size()),
      arm_kinematics::span<double>(runtime.solution_velocities.data(), runtime.solution_velocities.size()),
      kVelocityIkTimeStep);
    benchmark::DoNotOptimize(result);
    benchmark::DoNotOptimize(runtime.solution_velocities);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_TwistmapperGetVelocityIk);
