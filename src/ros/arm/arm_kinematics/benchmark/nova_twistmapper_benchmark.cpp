#include <benchmark/benchmark.h>

#include "arm_kinematics/inverse/inverse_kinematics_plugin.hpp"
#include "arm_kinematics/utilities/utilities.hpp"
#include "twistmapper_benchmark_support.hpp"

namespace {

}  // namespace

// ---------------------------------------------------------------------------
// FK benchmarks
// ---------------------------------------------------------------------------

// Models the per-cycle call that resolves the twist frame (resolve_base_twist).
static void BM_TwistmapperPositionFk(benchmark::State & state)
{
  arm_kinematics_benchmark::TwistmapperUpdateRuntime runtime("bm_tm_position_fk");

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
  arm_kinematics_benchmark::TwistmapperUpdateRuntime runtime("bm_tm_apply_twist_rotation");
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
  arm_kinematics_benchmark::TwistmapperUpdateRuntime runtime("bm_tm_apply_twist_linear");
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
  arm_kinematics_benchmark::TwistmapperUpdateRuntime runtime("bm_tm_make_tree");

  for (auto _ : state) {
    auto tree = arm_kinematics_benchmark::make_single_frame_tree(
      *runtime.fk,
      arm_kinematics_benchmark::kFallbackFrameId);
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
  arm_kinematics_benchmark::TwistmapperUpdateRuntime runtime("bm_tm_position_ik");

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
  arm_kinematics_benchmark::TwistmapperUpdateRuntime runtime("bm_tm_velocity_ik");

  for (auto _ : state) {
    auto result = runtime.ik->get_velocity_ik(
      runtime.twist_with_rotation,
      runtime.seed_ee_pose,
      arm_kinematics::span<const double>(runtime.current_joint_state.data(), runtime.current_joint_state.size()),
      arm_kinematics::span<double>(runtime.solution_velocities.data(), runtime.solution_velocities.size()),
      arm_kinematics_benchmark::kVelocityIkTimeStep);
    benchmark::DoNotOptimize(result);
    benchmark::DoNotOptimize(runtime.solution_velocities);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_TwistmapperGetVelocityIk);
