#include <benchmark/benchmark.h>

#include "arm_kinematics/collision/collision_config.hpp"
#include "arm_kinematics/collision/collision_manager.hpp"
#include "arm_kinematics/collision/collision_utilities.hpp"
#include "twistmapper_benchmark_support.hpp"

namespace {

}  // namespace

static void BM_TwistmapperConfigureCollisionStack(benchmark::State & state)
{
  const auto urdf = arm_kinematics_benchmark::load_taipan_urdf();
  const auto urdf_path = arm_kinematics_benchmark::benchmark_data_path();

  // Node lives outside the loop — we're timing kinematics init, not DDS startup.
  auto node = arm_kinematics_benchmark::make_benchmark_node("bm_twistmapper_configure");
  const auto collision_config = arm_kinematics::read_collision_config(node->get_node_parameters_interface());

  for (auto _ : state) {
    arm_kinematics::PluginLoader loader(*node, urdf);

    auto fk = loader.make_fk();
    if (!fk) {
      state.SkipWithError("Failed to create FK plugin.");
      break;
    }

    auto collision_result = arm_kinematics::make_collision_manager(
      loader,
      fk,
      arm_kinematics_benchmark::kJointNames,
      collision_config);
    if (!collision_result) {
      state.SkipWithError(collision_result.error().format().c_str());
      break;
    }

    auto ee_tree = arm_kinematics_benchmark::make_single_frame_tree(
      *fk,
      arm_kinematics_benchmark::kEndEffectorLinkName);
    auto twist_tree = arm_kinematics_benchmark::make_single_frame_tree(
      *fk,
      arm_kinematics_benchmark::kFallbackFrameId);

    benchmark::DoNotOptimize(collision_result->parent_link_names());
    benchmark::DoNotOptimize(ee_tree);
    benchmark::DoNotOptimize(twist_tree);
  }

  state.SetLabel(urdf_path);
}
BENCHMARK(BM_TwistmapperConfigureCollisionStack);

static void BM_TwistmapperCheckPathCollision(benchmark::State & state)
{
  arm_kinematics_benchmark::TwistmapperCollisionRuntime runtime("bm_twistmapper_runtime_path");

  for (auto _ : state) {
    auto collision_result = arm_kinematics::check_path_collision(
      runtime.collision_manager,
      runtime.current_joint_state_values,
      runtime.predicted_joint_positions,
      arm_kinematics_benchmark::kSelfIntersectionMaxStepSize,
      runtime.joint_values_scratch,
      runtime.colliding_pairs_scratch);
    benchmark::DoNotOptimize(collision_result);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_TwistmapperCheckPathCollision);

static void BM_TwistmapperUpdateAndCollideSingleState(benchmark::State & state)
{
  arm_kinematics_benchmark::TwistmapperCollisionRuntime runtime("bm_twistmapper_runtime_single_state");

  for (auto _ : state) {
    runtime.collision_manager.update_poses(runtime.predicted_joint_positions);
    const bool collided = runtime.collision_manager.collide(runtime.colliding_pairs_scratch);
    benchmark::DoNotOptimize(collided);
    benchmark::DoNotOptimize(runtime.colliding_pairs_scratch);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_TwistmapperUpdateAndCollideSingleState);
