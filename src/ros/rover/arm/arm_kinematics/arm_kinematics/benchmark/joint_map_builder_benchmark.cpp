//
// Created by Bailey Chessum on 12/4/26.
//
// Benchmarks for the joint map creation pipeline:
//   TransmissionReachability::analyze()  (fixed-point reachability)
//   plan_joint_map()                     (blueprint topological sort + segment emission)
//   materialize_joint_map()              (runtime object construction)
//   DefaultJointMapBuilder::build_expected()  (full pipeline)
//
// Topology families:
//   LinearChain(N)   — N joints, N-1 serial single-input/single-output transmissions.
//   AffineFan(N)     — N joints, joint 0 is the source, joints 1…N-1 mimic it.
//   Diamond(N)       — 1 source → N intermediates → 1 sink (fan-out then fan-in).
//
// Each family is parameterized with a joint count N passed via benchmark::Arg().

#include <benchmark/benchmark.h>

#include <memory>
#include <string>
#include <vector>

#include "arm_kinematics/joint_map/compute_transmission.hpp"
#include "arm_kinematics/joint_map/default_joint_map_builder.hpp"
#include "arm_kinematics/joint_map/joint_map_blueprint.hpp"
#include "arm_kinematics/joint_map/materialize_joint_map.hpp"
#include "arm_kinematics/joint_map/missing_input_resolution.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_model.hpp"
#include "arm_kinematics/joint_map/transmission_reachability.hpp"
#include "arm_kinematics/utilities/interface_id.hpp"

using arm_kinematics::ComputeTransmission;
using arm_kinematics::DefaultJointMapBuilder;
using arm_kinematics::InterfaceId;
using arm_kinematics::JointId;
using arm_kinematics::NamedStateInterfaceDefinition;
using arm_kinematics::StateInterfaceDefinition;
using arm_kinematics::StateInterfaceId;
using arm_kinematics::TransmissionAnalysis;
using arm_kinematics::TransmissionModel;
using arm_kinematics::TransmissionModelId;
using arm_kinematics::diagnose_missing_outputs;
using arm_kinematics::materialize_joint_map;
using arm_kinematics::plan_joint_map;
using arm_kinematics::span;

// ---------------------------------------------------------------------------
// Null transmission (zero-cost stand-in for real models)
// ---------------------------------------------------------------------------

namespace {

class NullCompute : public ComputeTransmission {
public:
  void compute(
    span<const double>,
    span<double>,
    span<double>) const override {}

  [[nodiscard]] std::size_t scratch_size() const noexcept override { return 0; }

  [[nodiscard]] std::unique_ptr<ComputeTransmission> clone() const override
  {
    return std::make_unique<NullCompute>();
  }
};

class NullModel : public TransmissionModel {
public:
  [[nodiscard]] std::unique_ptr<TransmissionModel> clone() const override
  {
    return std::make_unique<NullModel>();
  }

  [[nodiscard]] std::unique_ptr<const ComputeTransmission> build(
    span<const StateInterfaceId>,
    span<const StateInterfaceId>) const override
  {
    return std::make_unique<NullCompute>();
  }
};

// ---------------------------------------------------------------------------
// Topology factories
// ---------------------------------------------------------------------------

// N joints, N-1 single-input single-output transmissions in a serial chain:
//   joint_0 -T0-> joint_1 -T1-> joint_2 ... -T(N-2)-> joint_(N-1)
// Input: joint_0/position.  Output: joint_(N-1)/position.
struct LinearChain {
  TransmissionAnalysis analysis;
  std::vector<StateInterfaceDefinition> inputs;
  std::vector<StateInterfaceDefinition> outputs;

  explicit LinearChain(int n)
  {
    const auto model_id = analysis.add_model(std::make_unique<NullModel>());
    const InterfaceId pos{"position"};

    std::vector<JointId> joints;
    joints.reserve(n);
    for (int i = 0; i < n; ++i) {
      joints.push_back(analysis.ensure_joint_id("j" + std::to_string(i)));
    }

    std::vector<StateInterfaceId> sids;
    sids.reserve(n);
    for (int i = 0; i < n; ++i) {
      sids.push_back(analysis.ensure_state_interface_id({joints[i], pos}));
    }

    for (int i = 0; i + 1 < n; ++i) {
      analysis.add_transmission(model_id, {sids[i]}, {sids[i + 1]}, "T" + std::to_string(i));
    }

    inputs = {StateInterfaceDefinition{joints[0], pos}};
    outputs = {StateInterfaceDefinition{joints[n - 1], pos}};
  }
};

// N joints, joint 0 is the affine-group source, joints 1…N-1 mimic it.
// Input: joint_0/position.  Output: all joints/position.
struct AffineFan {
  TransmissionAnalysis analysis;
  std::vector<StateInterfaceDefinition> inputs;
  std::vector<StateInterfaceDefinition> outputs;

  explicit AffineFan(int n)
  {
    const InterfaceId pos{"position"};

    std::vector<JointId> joints;
    joints.reserve(n);
    for (int i = 0; i < n; ++i) {
      joints.push_back(analysis.ensure_joint_id("j" + std::to_string(i)));
    }

    for (int i = 1; i < n; ++i) {
      analysis.add_affine_transmission(joints[0], joints[i], 1.0, 0.0);
    }

    // Register SIDs so outputs are reachable via SID-keyed producer assignment.
    for (int i = 0; i < n; ++i) {
      analysis.ensure_state_interface_id({joints[i], pos});
    }

    inputs = {StateInterfaceDefinition{joints[0], pos}};
    outputs.reserve(n);
    for (int i = 0; i < n; ++i) {
      outputs.push_back(StateInterfaceDefinition{joints[i], pos});
    }
  }
};

// 1 source + N intermediates + 1 sink.
//   source -T0-> intermediate_0
//   source -T1-> intermediate_1
//   ...
//   source -T(N-1)-> intermediate_(N-1)
//   intermediate_0…(N-1) -T_N-> sink
// Input: source/position.  Output: sink/position.
struct Diamond {
  TransmissionAnalysis analysis;
  std::vector<StateInterfaceDefinition> inputs;
  std::vector<StateInterfaceDefinition> outputs;

  explicit Diamond(int n_intermediates)
  {
    const auto model_id = analysis.add_model(std::make_unique<NullModel>());
    const InterfaceId pos{"position"};

    const JointId src = analysis.ensure_joint_id("src");
    const JointId sink = analysis.ensure_joint_id("sink");
    const StateInterfaceId src_sid = analysis.ensure_state_interface_id({src, pos});
    const StateInterfaceId sink_sid = analysis.ensure_state_interface_id({sink, pos});

    std::vector<StateInterfaceId> intermediate_sids;
    intermediate_sids.reserve(n_intermediates);
    for (int i = 0; i < n_intermediates; ++i) {
      JointId j = analysis.ensure_joint_id("m" + std::to_string(i));
      intermediate_sids.push_back(analysis.ensure_state_interface_id({j, pos}));
      analysis.add_transmission(model_id, {src_sid}, {intermediate_sids.back()}, "T" + std::to_string(i));
    }

    analysis.add_transmission(model_id, intermediate_sids, {sink_sid}, "T_sink");

    inputs = {StateInterfaceDefinition{src, pos}};
    outputs = {StateInterfaceDefinition{sink, pos}};
  }
};

}  // namespace

// ---------------------------------------------------------------------------
// Reachability benchmarks
// ---------------------------------------------------------------------------

static void BM_Reachability_LinearChain(benchmark::State & state)
{
  const int n = static_cast<int>(state.range(0));
  LinearChain topo(n);
  for (auto _ : state) {
    auto reach = arm_kinematics::TransmissionReachability::analyze(topo.analysis, topo.inputs);
    benchmark::DoNotOptimize(reach);
  }
}
BENCHMARK(BM_Reachability_LinearChain)->Arg(8)->Arg(32)->Arg(128);

static void BM_Reachability_AffineFan(benchmark::State & state)
{
  const int n = static_cast<int>(state.range(0));
  AffineFan topo(n);
  for (auto _ : state) {
    auto reach = arm_kinematics::TransmissionReachability::analyze(topo.analysis, topo.inputs);
    benchmark::DoNotOptimize(reach);
  }
}
BENCHMARK(BM_Reachability_AffineFan)->Arg(8)->Arg(32)->Arg(128);

static void BM_Reachability_Diamond(benchmark::State & state)
{
  const int n = static_cast<int>(state.range(0));
  Diamond topo(n);
  for (auto _ : state) {
    auto reach = arm_kinematics::TransmissionReachability::analyze(topo.analysis, topo.inputs);
    benchmark::DoNotOptimize(reach);
  }
}
BENCHMARK(BM_Reachability_Diamond)->Arg(8)->Arg(32)->Arg(128);

// ---------------------------------------------------------------------------
// plan_joint_map benchmarks
// ---------------------------------------------------------------------------

static void BM_PlanJointMap_LinearChain(benchmark::State & state)
{
  const int n = static_cast<int>(state.range(0));
  LinearChain topo(n);
  const auto reach = arm_kinematics::TransmissionReachability::analyze(topo.analysis, topo.inputs);
  for (auto _ : state) {
    auto bp = plan_joint_map(reach, topo.outputs);
    benchmark::DoNotOptimize(bp);
  }
}
BENCHMARK(BM_PlanJointMap_LinearChain)->Arg(8)->Arg(32)->Arg(128);

static void BM_PlanJointMap_Diamond(benchmark::State & state)
{
  const int n = static_cast<int>(state.range(0));
  Diamond topo(n);
  const auto reach = arm_kinematics::TransmissionReachability::analyze(topo.analysis, topo.inputs);
  for (auto _ : state) {
    auto bp = plan_joint_map(reach, topo.outputs);
    benchmark::DoNotOptimize(bp);
  }
}
BENCHMARK(BM_PlanJointMap_Diamond)->Arg(8)->Arg(32)->Arg(128);

// ---------------------------------------------------------------------------
// materialize_joint_map benchmarks
// ---------------------------------------------------------------------------

static void BM_Materialize_LinearChain(benchmark::State & state)
{
  const int n = static_cast<int>(state.range(0));
  LinearChain topo(n);
  const auto reach = arm_kinematics::TransmissionReachability::analyze(topo.analysis, topo.inputs);
  const auto bp = plan_joint_map(reach, topo.outputs);
  for (auto _ : state) {
    auto jm = materialize_joint_map(bp, topo.analysis);
    benchmark::DoNotOptimize(jm);
  }
}
BENCHMARK(BM_Materialize_LinearChain)->Arg(8)->Arg(32)->Arg(128);

static void BM_Materialize_AffineFan(benchmark::State & state)
{
  const int n = static_cast<int>(state.range(0));
  AffineFan topo(n);
  const auto reach = arm_kinematics::TransmissionReachability::analyze(topo.analysis, topo.inputs);
  const auto bp = plan_joint_map(reach, topo.outputs);
  for (auto _ : state) {
    auto jm = materialize_joint_map(bp, topo.analysis);
    benchmark::DoNotOptimize(jm);
  }
}
BENCHMARK(BM_Materialize_AffineFan)->Arg(8)->Arg(32)->Arg(128);

// ---------------------------------------------------------------------------
// Full pipeline benchmarks (DefaultJointMapBuilder::build_expected)
// ---------------------------------------------------------------------------

static void BM_FullPipeline_LinearChain(benchmark::State & state)
{
  const int n = static_cast<int>(state.range(0));
  LinearChain topo(n);
  DefaultJointMapBuilder builder(topo.analysis);
  for (auto _ : state) {
    auto result = builder.build_expected(topo.inputs, topo.outputs);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_FullPipeline_LinearChain)->Arg(8)->Arg(32)->Arg(128);

static void BM_FullPipeline_AffineFan(benchmark::State & state)
{
  const int n = static_cast<int>(state.range(0));
  AffineFan topo(n);
  DefaultJointMapBuilder builder(topo.analysis);
  for (auto _ : state) {
    auto result = builder.build_expected(topo.inputs, topo.outputs);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_FullPipeline_AffineFan)->Arg(8)->Arg(32)->Arg(128);

static void BM_FullPipeline_Diamond(benchmark::State & state)
{
  const int n = static_cast<int>(state.range(0));
  Diamond topo(n);
  DefaultJointMapBuilder builder(topo.analysis);
  for (auto _ : state) {
    auto result = builder.build_expected(topo.inputs, topo.outputs);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_FullPipeline_Diamond)->Arg(8)->Arg(32)->Arg(128);
