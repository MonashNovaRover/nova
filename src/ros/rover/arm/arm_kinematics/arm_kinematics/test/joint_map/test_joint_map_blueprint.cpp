//
// Created by Bailey Chessum on 8/4/26.
//
// Unit tests for JointMapBlueprint and the diagnose_missing_outputs helper. These cover the
// output-dependent half of the analysis: which outputs are unreachable/ambiguous, and how
// the blueprint groups output production into segments (input/affine batches and transmission
// stages) with proper topological ordering and affine consolidation.
//

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <vector>

#include "arm_kinematics/joint_map/joint_map_blueprint.hpp"
#include "arm_kinematics/joint_map/missing_input_resolution.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_model.hpp"
#include "arm_kinematics/joint_map/compute_transmission.hpp"
#include "arm_kinematics/joint_map/transmission_reachability.hpp"

using arm_kinematics::ComputeTransmission;
using arm_kinematics::InterfaceId;
using arm_kinematics::JointId;
using arm_kinematics::JointMapBlueprint;
using arm_kinematics::JointMapBlueprintSegment;
using arm_kinematics::MissingOutputDiagnosis;
using arm_kinematics::NamedStateInterfaceDefinition;
using arm_kinematics::StateInterfaceId;
using arm_kinematics::TransmissionAnalysis;
using arm_kinematics::TransmissionInstanceId;
using arm_kinematics::TransmissionModel;
using arm_kinematics::TransmissionReachability;

namespace {

class StubComputeTransmission : public ComputeTransmission {
public:
  void compute(
    arm_kinematics::span<const float>,
    arm_kinematics::span<float>,
    arm_kinematics::span<float>) const override
  {
  }

  [[nodiscard]] size_t scratch_size() const noexcept override { return 0; }

  [[nodiscard]] std::unique_ptr<ComputeTransmission> clone() const override
  {
    return std::make_unique<StubComputeTransmission>();
  }
};

class StubTransmissionModel : public TransmissionModel {
public:
  [[nodiscard]] std::unique_ptr<TransmissionModel> clone() const override
  {
    return std::make_unique<StubTransmissionModel>();
  }

  [[nodiscard]] std::unique_ptr<const ComputeTransmission> build(
    arm_kinematics::span<const StateInterfaceId>,
    arm_kinematics::span<const StateInterfaceId>) const override
  {
    return std::make_unique<StubComputeTransmission>();
  }
};

std::vector<JointId> ensure_joints(
  TransmissionAnalysis & analysis,
  std::initializer_list<const char *> names)
{
  std::vector<JointId> ids;
  ids.reserve(names.size());
  for (const auto * name : names) {
    ids.push_back(analysis.ensure_joint_id(name));
  }
  return ids;
}

StateInterfaceId ensure_state(
  TransmissionAnalysis & analysis,
  const std::string & joint_name,
  const InterfaceId & iface)
{
  return analysis.ensure_state_interface_id(NamedStateInterfaceDefinition{joint_name, iface});
}

const JointMapBlueprintSegment::InputAffineBatch * as_affine_batch(
  const JointMapBlueprintSegment & seg)
{
  return std::get_if<JointMapBlueprintSegment::InputAffineBatch>(&seg.kind);
}

const JointMapBlueprintSegment::TransmissionStage * as_transmission_stage(
  const JointMapBlueprintSegment & seg)
{
  return std::get_if<JointMapBlueprintSegment::TransmissionStage>(&seg.kind);
}

constexpr float kFloatTolerance = 1.0e-5F;

bool approx_equal(float a, float b)
{
  return std::abs(a - b) <= kFloatTolerance + kFloatTolerance * std::max(std::abs(a), std::abs(b));
}

}  // namespace

class JointMapBlueprintTest : public ::testing::Test
{
protected:
  TransmissionAnalysis analysis_{};
};

// ===========================================================================
// diagnose_missing_outputs
// ===========================================================================

TEST_F(JointMapBlueprintTest, Diagnose_AllOutputsSatisfied_EmptyDiagnosis)
{
  ensure_joints(analysis_, {"j_a", "j_b"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {b}, "T");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceId>{a});
  const auto diag = diagnose_missing_outputs(reach, std::vector<StateInterfaceId>{a, b});

  EXPECT_TRUE(diag.unreachable.empty());
  EXPECT_TRUE(diag.ambiguous_outputs.empty());
}

TEST_F(JointMapBlueprintTest, Diagnose_UnreachableOutput_ReportedInUnreachableList)
{
  ensure_joints(analysis_, {"j_a", "j_b", "j_c"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis_, "j_c", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a, b}, {c}, "T");

  // Only `a` supplied → c is unreachable.
  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceId>{a});
  const auto diag = diagnose_missing_outputs(reach, std::vector<StateInterfaceId>{c});

  ASSERT_EQ(diag.unreachable.size(), 1u);
  EXPECT_EQ(diag.unreachable[0], c);
  EXPECT_TRUE(diag.ambiguous_outputs.empty());
}

TEST_F(JointMapBlueprintTest, Diagnose_TransitiveAmbiguity_OutputDownstreamOfAmbiguousIsTainted)
{
  // T1: a → x;  T2: a → x  (x is ambiguous)
  // T3: x → y                  (y depends on x; the transitive walk should taint y)
  // Inputs: {a}, Outputs: {y}
  // Expected: y in ambiguous_outputs, x in relevant_blocking_ambiguities, no unreachable.
  ensure_joints(analysis_, {"j_a", "j_x", "j_y"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});
  const auto y = ensure_state(analysis_, "j_y", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T1");
  analysis_.add_transmission(model_id, {a}, {x}, "T2");
  analysis_.add_transmission(model_id, {x}, {y}, "T3");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceId>{a});
  const auto diag = diagnose_missing_outputs(reach, std::vector<StateInterfaceId>{y});

  EXPECT_TRUE(diag.unreachable.empty());
  ASSERT_EQ(diag.ambiguous_outputs.size(), 1u);
  EXPECT_EQ(diag.ambiguous_outputs[0], y);
  ASSERT_EQ(diag.relevant_blocking_ambiguities.size(), 1u);
  EXPECT_EQ(diag.relevant_blocking_ambiguities[0].interface, x);
}

TEST_F(JointMapBlueprintTest, Diagnose_UnrelatedAmbiguity_DoesNotAffectRequest)
{
  // T1: a → x;  T2: a → x  (x is ambiguous, but unused)
  // T3: a → z                  (z is unique, no dependency on x)
  // Inputs: {a}, Outputs: {z}
  // Expected: empty diagnosis. The x-ambiguity is unrelated to the request.
  ensure_joints(analysis_, {"j_a", "j_x", "j_z"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  ensure_state(analysis_, "j_x", InterfaceId{"position"});
  const auto z = ensure_state(analysis_, "j_z", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  const auto x_id = ensure_state(analysis_, "j_x", InterfaceId{"position"});
  analysis_.add_transmission(model_id, {a}, {x_id}, "T1");
  analysis_.add_transmission(model_id, {a}, {x_id}, "T2");
  analysis_.add_transmission(model_id, {a}, {z}, "T3");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceId>{a});
  const auto diag = diagnose_missing_outputs(reach, std::vector<StateInterfaceId>{z});

  EXPECT_TRUE(reach.is_ambiguous());  // x is ambiguous in the reachability
  EXPECT_TRUE(diag.unreachable.empty());
  EXPECT_TRUE(diag.ambiguous_outputs.empty());  // ...but z is fine
  EXPECT_TRUE(diag.relevant_blocking_ambiguities.empty());
}

TEST_F(JointMapBlueprintTest, Diagnose_AmbiguousOutput_ReportedInAmbiguousList)
{
  ensure_joints(analysis_, {"j_a", "j_x"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T1");
  analysis_.add_transmission(model_id, {a}, {x}, "T2");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceId>{a});
  const auto diag = diagnose_missing_outputs(reach, std::vector<StateInterfaceId>{x});

  EXPECT_TRUE(diag.unreachable.empty());
  ASSERT_EQ(diag.ambiguous_outputs.size(), 1u);
  EXPECT_EQ(diag.ambiguous_outputs[0], x);
  ASSERT_EQ(diag.relevant_blocking_ambiguities.size(), 1u);
  EXPECT_EQ(diag.relevant_blocking_ambiguities[0].interface, x);
}

// ===========================================================================
// plan_joint_map: pure passthrough
// ===========================================================================

TEST_F(JointMapBlueprintTest, Plan_DirectInputPassthrough_OneAffineBatchSegment)
{
  // Inputs = outputs = {a}. The blueprint should have one InputAffineBatch with one identity
  // row.
  ensure_joints(analysis_, {"j_a"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceId>{a});
  const auto blueprint = plan_joint_map(reach, std::vector<StateInterfaceId>{a});

  ASSERT_EQ(blueprint.segments().size(), 1u);
  const auto * batch = as_affine_batch(blueprint.segments()[0]);
  ASSERT_NE(batch, nullptr);
  ASSERT_EQ(batch->blueprint_output_indices.size(), 1u);
  EXPECT_EQ(batch->blueprint_output_indices[0], 0u);
  EXPECT_EQ(batch->sources[0], a);
  EXPECT_TRUE(approx_equal(batch->multipliers[0], 1.0F));
  EXPECT_TRUE(approx_equal(batch->offsets[0], 0.0F));
}

// ===========================================================================
// plan_joint_map: affine batching consolidation (the load-bearing new behavior)
// ===========================================================================

TEST_F(JointMapBlueprintTest, AffineBatching_ConsecutiveAffineOutputs_FoldedIntoOneSegment)
{
  // Five mimic outputs all from one input. The blueprint should have ONE InputAffineBatch
  // with 5 rows, not 5 separate segments. (The plan-doc affine batching rule.)
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b1", "j_b2", "j_b3", "j_b4", "j_b5"});
  const auto a_pos = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b1 = ensure_state(analysis_, "j_b1", InterfaceId{"position"});
  const auto b2 = ensure_state(analysis_, "j_b2", InterfaceId{"position"});
  const auto b3 = ensure_state(analysis_, "j_b3", InterfaceId{"position"});
  const auto b4 = ensure_state(analysis_, "j_b4", InterfaceId{"position"});
  const auto b5 = ensure_state(analysis_, "j_b5", InterfaceId{"position"});

  for (std::size_t i = 1; i <= 5; ++i) {
    analysis_.add_affine_transmission(joints[0], joints[i], static_cast<float>(i), 0.0F);
  }

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceId>{a_pos});
  const auto blueprint = plan_joint_map(
    reach, std::vector<StateInterfaceId>{b1, b2, b3, b4, b5});

  ASSERT_EQ(blueprint.segments().size(), 1u);
  const auto * batch = as_affine_batch(blueprint.segments()[0]);
  ASSERT_NE(batch, nullptr);
  EXPECT_EQ(batch->blueprint_output_indices.size(), 5u);
  // Each row reads from a_pos with the right multiplier.
  for (std::size_t i = 0; i < 5; ++i) {
    EXPECT_EQ(batch->sources[i], a_pos);
    EXPECT_TRUE(approx_equal(batch->multipliers[i], static_cast<float>(i + 1)));
    EXPECT_TRUE(approx_equal(batch->offsets[i], 0.0F));
  }
}

TEST_F(JointMapBlueprintTest, AffineBatching_AffineThenTransmissionThenAffine)
{
  // Mimic edge: b mimics a (b = 2a)
  // Transmission: T: a → x
  // Mimic edge: y mimics x (y = 3x)
  // Request: outputs = {b, x, y}
  // Expected segment sequence:
  //   [InputAffineBatch with b] (pre-stage, sourced from a)
  //   [TransmissionStage T producing x, with x mapped to its blueprint output index]
  //   [InputAffineBatch with y] (post-T stage, sourced from x)
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b", "j_x", "j_y"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});
  const auto y = ensure_state(analysis_, "j_y", InterfaceId{"position"});

  analysis_.add_affine_transmission(joints[0], joints[1], 2.0F, 0.0F);
  analysis_.add_affine_transmission(joints[2], joints[3], 3.0F, 0.0F);

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceId>{a});
  const auto blueprint = plan_joint_map(
    reach, std::vector<StateInterfaceId>{b, x, y});

  ASSERT_EQ(blueprint.segments().size(), 3u);

  // Segment 0: pre-batch with b
  const auto * pre = as_affine_batch(blueprint.segments()[0]);
  ASSERT_NE(pre, nullptr);
  ASSERT_EQ(pre->blueprint_output_indices.size(), 1u);
  EXPECT_EQ(pre->blueprint_output_indices[0], 0u);  // b is at output position 0
  EXPECT_EQ(pre->sources[0], a);
  EXPECT_TRUE(approx_equal(pre->multipliers[0], 2.0F));

  // Segment 1: transmission stage T
  const auto * tstage = as_transmission_stage(blueprint.segments()[1]);
  ASSERT_NE(tstage, nullptr);
  EXPECT_EQ(tstage->instance_id, 0u);
  ASSERT_EQ(tstage->outputs.size(), 1u);
  EXPECT_EQ(tstage->outputs[0], x);
  ASSERT_EQ(tstage->blueprint_output_indices.size(), 1u);
  ASSERT_TRUE(tstage->blueprint_output_indices[0].has_value());
  EXPECT_EQ(*tstage->blueprint_output_indices[0], 1u);  // x is at output position 1

  // Segment 2: post-batch with y, sourced from x (a transmission output)
  const auto * post = as_affine_batch(blueprint.segments()[2]);
  ASSERT_NE(post, nullptr);
  ASSERT_EQ(post->blueprint_output_indices.size(), 1u);
  EXPECT_EQ(post->blueprint_output_indices[0], 2u);  // y is at output position 2
  EXPECT_EQ(post->sources[0], x);
  EXPECT_TRUE(approx_equal(post->multipliers[0], 3.0F));
}

TEST_F(JointMapBlueprintTest, InputPassthrough_FoldedIntoSameAffineBatchAsMimic)
{
  // Outputs include both a direct passthrough (a itself) and a mimic (b mimics a). They both
  // sit in the pre-transmission stage and should be folded into one InputAffineBatch.
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0F, 5.0F);

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceId>{a});
  const auto blueprint = plan_joint_map(reach, std::vector<StateInterfaceId>{a, b});

  ASSERT_EQ(blueprint.segments().size(), 1u);
  const auto * batch = as_affine_batch(blueprint.segments()[0]);
  ASSERT_NE(batch, nullptr);
  ASSERT_EQ(batch->blueprint_output_indices.size(), 2u);
  // Row for a: identity passthrough
  EXPECT_EQ(batch->blueprint_output_indices[0], 0u);
  EXPECT_EQ(batch->sources[0], a);
  EXPECT_TRUE(approx_equal(batch->multipliers[0], 1.0F));
  EXPECT_TRUE(approx_equal(batch->offsets[0], 0.0F));
  // Row for b: 2a + 5
  EXPECT_EQ(batch->blueprint_output_indices[1], 1u);
  EXPECT_EQ(batch->sources[1], a);
  EXPECT_TRUE(approx_equal(batch->multipliers[1], 2.0F));
  EXPECT_TRUE(approx_equal(batch->offsets[1], 5.0F));
}

// ===========================================================================
// plan_joint_map: side-effect outputs
// ===========================================================================

TEST_F(JointMapBlueprintTest, SideEffectOutput_TransmissionStageIncludesAllOutputs_BlueprintSelectsRequestedSubset)
{
  // T: a, b → i, k. User supplies a, b. Asks for k only (not i).
  // Expected: TransmissionStage with outputs = {i, k}, blueprint_output_indices = {nullopt, 0}
  // (k maps to blueprint output index 0; i is computed but not exposed).
  ensure_joints(analysis_, {"j_a", "j_b", "j_i", "j_k"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto i_iface = ensure_state(analysis_, "j_i", InterfaceId{"position"});
  const auto k = ensure_state(analysis_, "j_k", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a, b}, {i_iface, k}, "T");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceId>{a, b});
  const auto blueprint = plan_joint_map(reach, std::vector<StateInterfaceId>{k});

  ASSERT_EQ(blueprint.segments().size(), 1u);
  const auto * tstage = as_transmission_stage(blueprint.segments()[0]);
  ASSERT_NE(tstage, nullptr);
  ASSERT_EQ(tstage->outputs.size(), 2u);
  ASSERT_EQ(tstage->blueprint_output_indices.size(), 2u);

  // Find i and k in the outputs vector.
  for (std::size_t out_pos = 0; out_pos < tstage->outputs.size(); ++out_pos) {
    if (tstage->outputs[out_pos] == k) {
      ASSERT_TRUE(tstage->blueprint_output_indices[out_pos].has_value());
      EXPECT_EQ(*tstage->blueprint_output_indices[out_pos], 0u);
    } else if (tstage->outputs[out_pos] == i_iface) {
      EXPECT_FALSE(tstage->blueprint_output_indices[out_pos].has_value());
    }
  }
}

// ===========================================================================
// plan_joint_map: topological ordering
// ===========================================================================

TEST_F(JointMapBlueprintTest, TopologicalOrdering_SerialChain_T1BeforeT2)
{
  // T1: a → b
  // T2: b → c
  // Request: outputs = {c}. Both stages needed; T1 before T2.
  ensure_joints(analysis_, {"j_a", "j_b", "j_c"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis_, "j_c", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  const TransmissionInstanceId t1 = 0;
  const TransmissionInstanceId t2 = 1;
  analysis_.add_transmission(model_id, {a}, {b}, "T1");
  analysis_.add_transmission(model_id, {b}, {c}, "T2");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceId>{a});
  const auto blueprint = plan_joint_map(reach, std::vector<StateInterfaceId>{c});

  // Two transmission stages (in addition to any empty pre-batches that get skipped).
  std::vector<TransmissionInstanceId> stage_order;
  for (const auto & seg : blueprint.segments()) {
    if (auto * t = as_transmission_stage(seg)) {
      stage_order.push_back(t->instance_id);
    }
  }
  ASSERT_EQ(stage_order.size(), 2u);
  EXPECT_EQ(stage_order[0], t1);
  EXPECT_EQ(stage_order[1], t2);
}

TEST_F(JointMapBlueprintTest, TopologicalOrdering_DiamondDependencies_T3LastBothPredecessorsBefore)
{
  // T1: a → b
  // T2: a → c
  // T3: b, c → d
  // Request: outputs = {d}. Both T1 and T2 must come before T3.
  ensure_joints(analysis_, {"j_a", "j_b", "j_c", "j_d"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis_, "j_c", InterfaceId{"position"});
  const auto d = ensure_state(analysis_, "j_d", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  const TransmissionInstanceId t1 = 0;
  const TransmissionInstanceId t2 = 1;
  const TransmissionInstanceId t3 = 2;
  analysis_.add_transmission(model_id, {a}, {b}, "T1");
  analysis_.add_transmission(model_id, {a}, {c}, "T2");
  analysis_.add_transmission(model_id, {b, c}, {d}, "T3");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceId>{a});
  const auto blueprint = plan_joint_map(reach, std::vector<StateInterfaceId>{d});

  std::vector<TransmissionInstanceId> stage_order;
  for (const auto & seg : blueprint.segments()) {
    if (auto * t = as_transmission_stage(seg)) {
      stage_order.push_back(t->instance_id);
    }
  }
  ASSERT_EQ(stage_order.size(), 3u);

  auto pos_of = [&](TransmissionInstanceId target) -> std::size_t {
    for (std::size_t i = 0; i < stage_order.size(); ++i) {
      if (stage_order[i] == target) return i;
    }
    return static_cast<std::size_t>(-1);
  };
  const auto p1 = pos_of(t1);
  const auto p2 = pos_of(t2);
  const auto p3 = pos_of(t3);
  ASSERT_NE(p1, static_cast<std::size_t>(-1));
  ASSERT_NE(p2, static_cast<std::size_t>(-1));
  ASSERT_NE(p3, static_cast<std::size_t>(-1));
  EXPECT_LT(p1, p3);
  EXPECT_LT(p2, p3);
}
