//
// Created by Bailey Chessum on 8/4/26.
//
// Unit tests for TransmissionSubgraph against synthetic TransmissionAnalysis instances.
// No FK dependency — these tests build the analysis up from raw joints, transmission models,
// and affine relationships, then assert on the subgraph's algorithm output.
//

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <vector>

#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_model.hpp"
#include "arm_kinematics/joint_map/compute_transmission.hpp"
#include "arm_kinematics/joint_map/transmission_subgraph.hpp"

using arm_kinematics::AffineProjectionRule;
using arm_kinematics::ComputeTransmission;
using arm_kinematics::InterfaceId;
using arm_kinematics::JointId;
using arm_kinematics::NamedStateInterfaceDefinition;
using arm_kinematics::StateInterfaceDefinition;
using arm_kinematics::StateInterfaceId;
using arm_kinematics::StateInterfaceProducer;
using arm_kinematics::TransmissionAnalysis;
using arm_kinematics::TransmissionInstanceId;
using arm_kinematics::TransmissionModel;
using arm_kinematics::TransmissionSubgraph;

namespace producers = arm_kinematics::producers;

namespace {

// ---------------------------------------------------------------------------
// Test mocks
// ---------------------------------------------------------------------------

// A trivial ComputeTransmission that does nothing. The subgraph algorithm only cares about
// shape (which transmissions are viable based on their input/output sets), not behavior, so
// the compute() body is never invoked from these tests.
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

// A trivial TransmissionModel that returns a stub ComputeTransmission for any (input, output)
// pair the analysis registers. Keeping this minimal — it doesn't enforce any (input, output)
// shape constraints because the subgraph tests don't run compute.
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

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Builds an analysis with the given joints (by name). Returns the JointIds in the same order
// the names were passed.
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

// Convenience to ensure a state interface for a given joint name + interface id.
StateInterfaceId ensure_state(
  TransmissionAnalysis & analysis,
  const std::string & joint_name,
  const InterfaceId & iface)
{
  return analysis.ensure_state_interface_id(NamedStateInterfaceDefinition{joint_name, iface});
}

// Test helpers for asserting on producer variants.
//
// IMPORTANT: these return *copies* of the inner producer (via std::optional), NOT pointers.
// `subgraph.producer_of(...)` returns a StateInterfaceProducer by value — a temporary. If we
// returned a raw pointer into the temporary, the caller would get a dangling pointer once the
// temporary died at the end of the full expression. Returning a copy is safe.
std::optional<producers::Input> as_input(const StateInterfaceProducer & p)
{
  if (auto * x = std::get_if<producers::Input>(&p)) return *x;
  return std::nullopt;
}

std::optional<producers::Transmission> as_transmission(const StateInterfaceProducer & p)
{
  if (auto * x = std::get_if<producers::Transmission>(&p)) return *x;
  return std::nullopt;
}

std::optional<producers::AffineProjection> as_affine(const StateInterfaceProducer & p)
{
  if (auto * x = std::get_if<producers::AffineProjection>(&p)) return *x;
  return std::nullopt;
}

bool is_monostate(const StateInterfaceProducer & p)
{
  return std::holds_alternative<std::monostate>(p);
}

constexpr float kFloatTolerance = 1.0e-5F;

bool approx_equal(float a, float b)
{
  return std::abs(a - b) <= kFloatTolerance + kFloatTolerance * std::max(std::abs(a), std::abs(b));
}

}  // namespace

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class TransmissionSubgraphTest : public ::testing::Test
{
protected:
  TransmissionAnalysis analysis_{};
};

// ===========================================================================
// Pure transmission graph
// ===========================================================================

TEST_F(TransmissionSubgraphTest, PureTransmission_SingleProducer_PlanIsComplete)
{
  // a, b → T → c
  //   request:  inputs={a}, outputs={b, c}  ← unreachable, should fail
  //   request:  inputs={a, b}, outputs={c}  ← complete via T
  ensure_joints(analysis_, {"j_a", "j_b", "j_c"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis_, "j_c", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a, b}, {c}, "T");

  TransmissionSubgraph subgraph(analysis_, std::vector<StateInterfaceId>{a, b}, std::vector<StateInterfaceId>{c});

  EXPECT_TRUE(subgraph.is_complete());
  EXPECT_FALSE(subgraph.is_ambiguous());
  EXPECT_TRUE(subgraph.unreachable_outputs().empty());
  EXPECT_TRUE(subgraph.ambiguous_interfaces().empty());

  // Producer of a/b is Input.
  ASSERT_TRUE(as_input(subgraph.producer_of(a)).has_value());
  ASSERT_TRUE(as_input(subgraph.producer_of(b)).has_value());
  EXPECT_EQ(as_input(subgraph.producer_of(a))->input_index, 0u);
  EXPECT_EQ(as_input(subgraph.producer_of(b))->input_index, 1u);

  // Producer of c is the transmission.
  const auto c_producer = as_transmission(subgraph.producer_of(c));
  ASSERT_TRUE(c_producer.has_value());
  EXPECT_EQ(c_producer->instance_id, 0u);

  // The single transmission is selected.
  ASSERT_EQ(subgraph.selected_transmissions().size(), 1u);
  EXPECT_EQ(subgraph.selected_transmissions()[0], 0u);
}

TEST_F(TransmissionSubgraphTest, PureTransmission_MissingInput_ReportsUnreachableOutput)
{
  ensure_joints(analysis_, {"j_a", "j_b", "j_c"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis_, "j_c", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a, b}, {c}, "T");

  // Only supply a — b is missing, so c can't be derived.
  TransmissionSubgraph subgraph(analysis_, std::vector<StateInterfaceId>{a}, std::vector<StateInterfaceId>{c});

  EXPECT_FALSE(subgraph.is_complete());
  EXPECT_FALSE(subgraph.is_ambiguous());
  ASSERT_EQ(subgraph.unreachable_outputs().size(), 1u);
  EXPECT_EQ(subgraph.unreachable_outputs()[0], c);
  EXPECT_TRUE(is_monostate(subgraph.producer_of(c)));
  EXPECT_TRUE(subgraph.selected_transmissions().empty());
}

// ===========================================================================
// Affine projection — three rule types
// ===========================================================================

TEST_F(TransmissionSubgraphTest, AffineProjection_PositionRule_Composition)
{
  // Mimic chain: B = 2A + 5
  // Request: inputs={A.position}, outputs={B.position}
  // Expect: B is derivable via affine projection. Coefficients: m=2, o=5.
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b"});
  const auto a_pos = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b_pos = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0F, 5.0F);

  TransmissionSubgraph subgraph(
    analysis_, std::vector<StateInterfaceId>{a_pos}, std::vector<StateInterfaceId>{b_pos});

  EXPECT_TRUE(subgraph.is_complete());
  const auto proj = as_affine(subgraph.producer_of(b_pos));
  ASSERT_TRUE(proj.has_value());
  EXPECT_EQ(proj->source, a_pos);
  EXPECT_TRUE(approx_equal(proj->multiplier, 2.0F));
  EXPECT_TRUE(approx_equal(proj->offset, 5.0F));
}

TEST_F(TransmissionSubgraphTest, AffineProjection_VelocityRule_DropsOffset)
{
  // Mimic: B_pos = 2*A_pos + 5  →  B_vel = 2*A_vel (offset dropped)
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b"});
  const auto a_vel = ensure_state(analysis_, "j_a", InterfaceId{"velocity"});
  const auto b_vel = ensure_state(analysis_, "j_b", InterfaceId{"velocity"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0F, 5.0F);

  TransmissionSubgraph subgraph(
    analysis_, std::vector<StateInterfaceId>{a_vel}, std::vector<StateInterfaceId>{b_vel});

  EXPECT_TRUE(subgraph.is_complete());
  const auto proj = as_affine(subgraph.producer_of(b_vel));
  ASSERT_TRUE(proj.has_value());
  EXPECT_EQ(proj->source, a_vel);
  EXPECT_TRUE(approx_equal(proj->multiplier, 2.0F));
  EXPECT_TRUE(approx_equal(proj->offset, 0.0F));
}

TEST_F(TransmissionSubgraphTest, AffineProjection_EffortRule_RequiresExplicitRegistration)
{
  // Mimic: B_pos = 2*A_pos + 5
  // Default: no effort rule registered — affine doesn't propagate effort.
  // After explicit registration: effort propagates with reverse direction.
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b"});
  const auto a_eff = ensure_state(analysis_, "j_a", InterfaceId{"effort"});
  const auto b_eff = ensure_state(analysis_, "j_b", InterfaceId{"effort"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0F, 5.0F);

  // Without registering an effort rule, B.effort is unreachable from A.effort.
  {
    TransmissionSubgraph subgraph(
      analysis_, std::vector<StateInterfaceId>{a_eff}, std::vector<StateInterfaceId>{b_eff});
    EXPECT_FALSE(subgraph.is_complete());
    ASSERT_EQ(subgraph.unreachable_outputs().size(), 1u);
    EXPECT_EQ(subgraph.unreachable_outputs()[0], b_eff);
  }

  // Register the effort rule explicitly: τ_source = m·τ_target via energy conservation.
  analysis_.set_affine_projection_rule(
    InterfaceId{"effort"},
    AffineProjectionRule{1.0F, 0.0F, true});

  // Now from A.effort = 1, B.effort = ? : τ_a = m * τ_b → τ_b = τ_a / m = 0.5 * τ_a
  // The projection-rule implementation inverts when reverse_direction is true, so
  // (target = m * source + o) becomes (target = (1/m) * source - o/m). With m=2, o=0
  // (offset dropped by velocity-style rule scale 0): B.effort = 0.5 * A.effort.
  {
    TransmissionSubgraph subgraph(
      analysis_, std::vector<StateInterfaceId>{a_eff}, std::vector<StateInterfaceId>{b_eff});
    EXPECT_TRUE(subgraph.is_complete());
    const auto proj = as_affine(subgraph.producer_of(b_eff));
    ASSERT_TRUE(proj.has_value());
    EXPECT_EQ(proj->source, a_eff);
    EXPECT_TRUE(approx_equal(proj->multiplier, 0.5F));
    EXPECT_TRUE(approx_equal(proj->offset, 0.0F));
  }
}

// ===========================================================================
// Affine chain composition
// ===========================================================================

TEST_F(TransmissionSubgraphTest, AffineChain_ComposesAcrossThreeJoints)
{
  // A → B → C with B = 2A + 5, C = 3B + 7.
  // Composed: C = 3*(2A + 5) + 7 = 6A + 22.
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b", "j_c"});
  const auto a_pos = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b_pos = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto c_pos = ensure_state(analysis_, "j_c", InterfaceId{"position"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0F, 5.0F);
  analysis_.add_affine_transmission(joints[1], joints[2], 3.0F, 7.0F);

  TransmissionSubgraph subgraph(
    analysis_, std::vector<StateInterfaceId>{a_pos},
    std::vector<StateInterfaceId>{b_pos, c_pos});

  EXPECT_TRUE(subgraph.is_complete());
  const auto b_proj = as_affine(subgraph.producer_of(b_pos));
  ASSERT_TRUE(b_proj.has_value());
  EXPECT_TRUE(approx_equal(b_proj->multiplier, 2.0F));
  EXPECT_TRUE(approx_equal(b_proj->offset, 5.0F));
  const auto c_proj = as_affine(subgraph.producer_of(c_pos));
  ASSERT_TRUE(c_proj.has_value());
  EXPECT_TRUE(approx_equal(c_proj->multiplier, 6.0F));
  EXPECT_TRUE(approx_equal(c_proj->offset, 22.0F));
}

// ===========================================================================
// No projection rule = no propagation
// ===========================================================================

TEST_F(TransmissionSubgraphTest, AffineProjection_NoRuleForInterface_DoesNotPropagate)
{
  // Mimic A→B exists. The "custom_iface" interface id has no rule registered, so even though
  // A.custom and B.custom exist in the analysis, the algorithm refuses to project between them.
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b"});
  const auto a_custom = ensure_state(analysis_, "j_a", InterfaceId{"custom_iface"});
  const auto b_custom = ensure_state(analysis_, "j_b", InterfaceId{"custom_iface"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0F, 0.0F);

  TransmissionSubgraph subgraph(
    analysis_, std::vector<StateInterfaceId>{a_custom},
    std::vector<StateInterfaceId>{b_custom});

  EXPECT_FALSE(subgraph.is_complete());
  ASSERT_EQ(subgraph.unreachable_outputs().size(), 1u);
  EXPECT_EQ(subgraph.unreachable_outputs()[0], b_custom);
}

// ===========================================================================
// Viability filtering
// ===========================================================================

TEST_F(TransmissionSubgraphTest, ViabilityFiltering_TransmissionWithUnreachableInputsIsNotACandidate)
{
  // Two transmissions producing the same output, but one of them has unreachable inputs.
  // T1: a → x  (a is supplied)
  // T2: b → x  (b is NOT supplied)
  // Request: inputs={a}, outputs={x}.
  // Expect: T1 produces x. T2 is not a candidate (b unreachable). x is NOT ambiguous.
  ensure_joints(analysis_, {"j_a", "j_b", "j_x"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T1");
  analysis_.add_transmission(model_id, {b}, {x}, "T2");

  TransmissionSubgraph subgraph(analysis_, std::vector<StateInterfaceId>{a}, std::vector<StateInterfaceId>{x});

  EXPECT_TRUE(subgraph.is_complete());
  EXPECT_FALSE(subgraph.is_ambiguous());
  const auto x_producer = as_transmission(subgraph.producer_of(x));
  ASSERT_TRUE(x_producer.has_value());
  EXPECT_EQ(x_producer->instance_id, 0u);  // T1
}

// ===========================================================================
// add_input mutation
// ===========================================================================

TEST_F(TransmissionSubgraphTest, AddInput_UnblocksPreviouslyIncompletePlan)
{
  ensure_joints(analysis_, {"j_a", "j_b", "j_c"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis_, "j_c", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a, b}, {c}, "T");

  TransmissionSubgraph subgraph(analysis_, std::vector<StateInterfaceId>{a}, std::vector<StateInterfaceId>{c});
  EXPECT_FALSE(subgraph.is_complete());

  subgraph.add_input(b);

  EXPECT_TRUE(subgraph.is_complete());
  ASSERT_TRUE(as_transmission(subgraph.producer_of(c)).has_value());
  // No override entry — the prior state for `b` was monostate (not yet known), nothing
  // discarded.
  EXPECT_TRUE(subgraph.input_overrides().empty());
}

TEST_F(TransmissionSubgraphTest, AddInput_OverridesPreviousNonInputProducer)
{
  // T1: a → x. Initial state: a supplied, x derivable via T1. Then add_input(x) — overrides
  // T1's role as the producer of x.
  ensure_joints(analysis_, {"j_a", "j_x"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T1");

  TransmissionSubgraph subgraph(analysis_, std::vector<StateInterfaceId>{a}, std::vector<StateInterfaceId>{x});
  ASSERT_TRUE(as_transmission(subgraph.producer_of(x)).has_value());

  subgraph.add_input(x);

  // Now x's producer is Input, not T1.
  ASSERT_TRUE(as_input(subgraph.producer_of(x)).has_value());
  // Override log records the displacement.
  ASSERT_EQ(subgraph.input_overrides().size(), 1u);
  EXPECT_EQ(subgraph.input_overrides()[0].interface, x);
  ASSERT_EQ(subgraph.input_overrides()[0].discarded_candidates.size(), 1u);
  ASSERT_TRUE(as_transmission(subgraph.input_overrides()[0].discarded_candidates[0]).has_value());
}

TEST_F(TransmissionSubgraphTest, AddInput_DuplicateInterface_NoOverrideEntry)
{
  // a is in initial_inputs. add_input(a) — duplicate. No override, no producer change.
  ensure_joints(analysis_, {"j_a"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});

  TransmissionSubgraph subgraph(analysis_, std::vector<StateInterfaceId>{a}, std::vector<StateInterfaceId>{a});
  EXPECT_TRUE(subgraph.is_complete());
  EXPECT_EQ(as_input(subgraph.producer_of(a))->input_index, 0u);

  subgraph.add_input(a);

  // requested_inputs() now has two entries for `a`.
  ASSERT_EQ(subgraph.requested_inputs().size(), 2u);
  EXPECT_EQ(subgraph.requested_inputs()[0], a);
  EXPECT_EQ(subgraph.requested_inputs()[1], a);
  // producer_of(a) still points at the first occurrence (input_index == 0).
  EXPECT_EQ(as_input(subgraph.producer_of(a))->input_index, 0u);
  // No override entry.
  EXPECT_TRUE(subgraph.input_overrides().empty());
}

TEST_F(TransmissionSubgraphTest, DuplicateInterfaceInInitialInputs_FirstOccurrenceWins)
{
  ensure_joints(analysis_, {"j_a"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});

  TransmissionSubgraph subgraph(
    analysis_, std::vector<StateInterfaceId>{a, a}, std::vector<StateInterfaceId>{a});

  ASSERT_EQ(subgraph.requested_inputs().size(), 2u);
  EXPECT_EQ(subgraph.requested_inputs()[0], a);
  EXPECT_EQ(subgraph.requested_inputs()[1], a);
  EXPECT_EQ(as_input(subgraph.producer_of(a))->input_index, 0u);
}

// ===========================================================================
// Input wins
// ===========================================================================

TEST_F(TransmissionSubgraphTest, InputWinsOverDerivedAffineProducer)
{
  // B mimics A. User supplies BOTH A.position AND B.position. No false ambiguity from the
  // affine projection — both are produced by Input.
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b"});
  const auto a_pos = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b_pos = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0F, 5.0F);

  TransmissionSubgraph subgraph(
    analysis_,
    std::vector<StateInterfaceId>{a_pos, b_pos},
    std::vector<StateInterfaceId>{a_pos, b_pos});

  EXPECT_TRUE(subgraph.is_complete());
  EXPECT_FALSE(subgraph.is_ambiguous());
  ASSERT_TRUE(as_input(subgraph.producer_of(a_pos)).has_value());
  ASSERT_TRUE(as_input(subgraph.producer_of(b_pos)).has_value());
}

TEST_F(TransmissionSubgraphTest, InputWinsOverTransmission)
{
  // T: a → x. User supplies x as input directly. T is not selected for x.
  ensure_joints(analysis_, {"j_a", "j_x"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T");

  TransmissionSubgraph subgraph(
    analysis_, std::vector<StateInterfaceId>{a, x}, std::vector<StateInterfaceId>{x});
  EXPECT_TRUE(subgraph.is_complete());
  ASSERT_TRUE(as_input(subgraph.producer_of(x)).has_value());
  // T is NOT in selected_transmissions() because nothing else needs it.
  EXPECT_TRUE(subgraph.selected_transmissions().empty());
}

// ===========================================================================
// Ambiguity
// ===========================================================================

TEST_F(TransmissionSubgraphTest, AmbiguityAccumulation_MultipleAmbiguousInterfacesAllReported)
{
  // Two outputs each with two viable transmissions. Both should be reported as ambiguous.
  // T1: a → x      T2: a → x  (ambiguous for x)
  // T3: a → y      T4: a → y  (ambiguous for y)
  ensure_joints(analysis_, {"j_a", "j_x", "j_y"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});
  const auto y = ensure_state(analysis_, "j_y", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T1");
  analysis_.add_transmission(model_id, {a}, {x}, "T2");
  analysis_.add_transmission(model_id, {a}, {y}, "T3");
  analysis_.add_transmission(model_id, {a}, {y}, "T4");

  TransmissionSubgraph subgraph(
    analysis_, std::vector<StateInterfaceId>{a}, std::vector<StateInterfaceId>{x, y});

  EXPECT_TRUE(subgraph.is_ambiguous());
  EXPECT_FALSE(subgraph.is_complete());
  ASSERT_EQ(subgraph.ambiguous_interfaces().size(), 2u);
  // Both x and y are reported, with their candidate counts.
  for (const auto & ai : subgraph.ambiguous_interfaces()) {
    EXPECT_EQ(ai.candidates.size(), 2u);
    EXPECT_TRUE(ai.interface == x || ai.interface == y);
  }
  // producer_of for ambiguous interfaces returns monostate.
  EXPECT_TRUE(is_monostate(subgraph.producer_of(x)));
  EXPECT_TRUE(is_monostate(subgraph.producer_of(y)));
}

TEST_F(TransmissionSubgraphTest, NonAmbiguousInterfacesInOtherwiseAmbiguousPlan_StillReturnTheirProducer)
{
  // Plan has one ambiguous interface (x) and one non-ambiguous (z). z's producer is still
  // queryable.
  ensure_joints(analysis_, {"j_a", "j_x", "j_z"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});
  const auto z = ensure_state(analysis_, "j_z", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T1");
  analysis_.add_transmission(model_id, {a}, {x}, "T2");  // T2 makes x ambiguous
  analysis_.add_transmission(model_id, {a}, {z}, "T3");

  TransmissionSubgraph subgraph(
    analysis_, std::vector<StateInterfaceId>{a}, std::vector<StateInterfaceId>{x, z});

  EXPECT_TRUE(subgraph.is_ambiguous());
  EXPECT_TRUE(is_monostate(subgraph.producer_of(x)));
  // z is NOT ambiguous, has a unique producer.
  const auto z_producer = as_transmission(subgraph.producer_of(z));
  ASSERT_TRUE(z_producer.has_value());
}

// ===========================================================================
// Mixed-quantity transmission (input and output have different interface ids)
// ===========================================================================

TEST_F(TransmissionSubgraphTest, MixedQuantityTransmission_PositionInEffortOut)
{
  // T: a.position → x.effort. User supplies a.position, asks for x.effort.
  ensure_joints(analysis_, {"j_a", "j_x"});
  const auto a_pos = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x_eff = ensure_state(analysis_, "j_x", InterfaceId{"effort"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a_pos}, {x_eff}, "T");

  TransmissionSubgraph subgraph(
    analysis_, std::vector<StateInterfaceId>{a_pos}, std::vector<StateInterfaceId>{x_eff});

  EXPECT_TRUE(subgraph.is_complete());
  ASSERT_TRUE(as_transmission(subgraph.producer_of(x_eff)).has_value());
}

// ===========================================================================
// Side-effect outputs
// ===========================================================================

TEST_F(TransmissionSubgraphTest, SideEffectOutput_TransmissionStillSelectedForRequestedOutput)
{
  // T produces both K and I from {a, b}. User supplies a, b, I. Asks for K.
  // T is selected because it produces K. The runtime would compute I as a side-effect, but
  // producer_of(I) is Input (user wins). For the subgraph, we just check that T is selected.
  ensure_joints(analysis_, {"j_a", "j_b", "j_i", "j_k"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto i = ensure_state(analysis_, "j_i", InterfaceId{"position"});
  const auto k = ensure_state(analysis_, "j_k", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a, b}, {i, k}, "T");

  TransmissionSubgraph subgraph(
    analysis_, std::vector<StateInterfaceId>{a, b, i}, std::vector<StateInterfaceId>{k});

  EXPECT_TRUE(subgraph.is_complete());
  ASSERT_TRUE(as_input(subgraph.producer_of(i)).has_value());  // Input wins for I
  ASSERT_TRUE(as_transmission(subgraph.producer_of(k)).has_value());  // T produces K
  ASSERT_EQ(subgraph.selected_transmissions().size(), 1u);
}

// ===========================================================================
// Redundant equivalent inputs
// ===========================================================================

TEST_F(TransmissionSubgraphTest, RedundantEquivalentInputs_MultipleLeavesInSameAffineGroup)
{
  // B mimics A. User supplies both A.position and D.position where D mimics A as well.
  // Wait, let me set up properly: A→B (B mimics A), A→D (D mimics A). Now A, B, D are all
  // in the same affine group. User supplies A.position and D.position. The lowest-JointId
  // leaf (A, since A is added first) wins as the affine source for B; D becomes a redundant
  // equivalent input.
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b", "j_d"});
  const auto a_pos = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b_pos = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto d_pos = ensure_state(analysis_, "j_d", InterfaceId{"position"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0F, 0.0F);  // B = 2A
  analysis_.add_affine_transmission(joints[0], joints[2], 3.0F, 0.0F);  // D = 3A

  TransmissionSubgraph subgraph(
    analysis_,
    std::vector<StateInterfaceId>{a_pos, d_pos},
    std::vector<StateInterfaceId>{b_pos});

  EXPECT_TRUE(subgraph.is_complete());
  // B's producer should be AffineProjection sourced from A (the lowest JointId leaf).
  const auto b_proj = as_affine(subgraph.producer_of(b_pos));
  ASSERT_TRUE(b_proj.has_value());
  EXPECT_EQ(b_proj->source, a_pos);
  // D.position is in the redundant_equivalent_inputs list.
  ASSERT_EQ(subgraph.redundant_equivalent_inputs().size(), 1u);
  EXPECT_EQ(subgraph.redundant_equivalent_inputs()[0], d_pos);
  // D.position is still queryable as Input (the user's value is honored).
  ASSERT_TRUE(as_input(subgraph.producer_of(d_pos)).has_value());
}

// ===========================================================================
// derivable_interfaces and analysis() accessor
// ===========================================================================

TEST_F(TransmissionSubgraphTest, AnalysisAccessor_ReturnsTheSameAnalysis)
{
  ensure_joints(analysis_, {"j_a"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});

  TransmissionSubgraph subgraph(analysis_, std::vector<StateInterfaceId>{a}, std::vector<StateInterfaceId>{a});
  EXPECT_EQ(&subgraph.analysis(), &analysis_);
}

TEST_F(TransmissionSubgraphTest, DerivableInterfaces_IncludesInputsAndDerivedOutputs)
{
  // a → T → c. derivable should contain {a, c}.
  ensure_joints(analysis_, {"j_a", "j_c"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto c = ensure_state(analysis_, "j_c", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {c}, "T");

  TransmissionSubgraph subgraph(analysis_, std::vector<StateInterfaceId>{a}, std::vector<StateInterfaceId>{c});

  const auto derivable = subgraph.derivable_interfaces();
  ASSERT_EQ(derivable.size(), 2u);
  // a is added first (initial input), then c (derived).
  EXPECT_EQ(derivable[0], a);
  EXPECT_EQ(derivable[1], c);
}

// ===========================================================================
// Topological ordering of selected_transmissions
// ===========================================================================

TEST_F(TransmissionSubgraphTest, SelectedTransmissions_TopologicallySorted)
{
  // T1: a → b
  // T2: b → c
  // Request: inputs={a}, outputs={c}. Both T1 and T2 are needed; T1 must come before T2.
  ensure_joints(analysis_, {"j_a", "j_b", "j_c"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis_, "j_c", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  const TransmissionInstanceId t1 = 0;
  const TransmissionInstanceId t2 = 1;
  analysis_.add_transmission(model_id, {a}, {b}, "T1");
  analysis_.add_transmission(model_id, {b}, {c}, "T2");

  TransmissionSubgraph subgraph(analysis_, std::vector<StateInterfaceId>{a}, std::vector<StateInterfaceId>{c});

  EXPECT_TRUE(subgraph.is_complete());
  ASSERT_EQ(subgraph.selected_transmissions().size(), 2u);
  EXPECT_EQ(subgraph.selected_transmissions()[0], t1);
  EXPECT_EQ(subgraph.selected_transmissions()[1], t2);
}

TEST_F(TransmissionSubgraphTest, SelectedTransmissions_DiamondDependencies_PartialOrderRespected)
{
  // Diamond:
  //   T1: a → b
  //   T2: a → c
  //   T3: b, c → d
  // Request: inputs={a}, outputs={d}. All three needed; T3 must come last.
  // T1 and T2 may be in either relative order.
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

  TransmissionSubgraph subgraph(
    analysis_, std::vector<StateInterfaceId>{a}, std::vector<StateInterfaceId>{d});

  EXPECT_TRUE(subgraph.is_complete());
  const auto selected = subgraph.selected_transmissions();
  ASSERT_EQ(selected.size(), 3u);

  // Determine positions of each transmission in the order.
  auto pos_of = [&](TransmissionInstanceId target) -> std::size_t {
    for (std::size_t i = 0; i < selected.size(); ++i) {
      if (selected[i] == target) return i;
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

// ===========================================================================
// Affine projection sourced from a transmission output (leaf-source invariant)
// ===========================================================================

TEST_F(TransmissionSubgraphTest, AffineProjection_SourcedFromTransmissionOutput_ResolvesToLeaf)
{
  // T: a → x.position. Mimic edge x → y (y mimics x with multiplier 2).
  // Request: inputs={a}, outputs={x.position, y.position}.
  // Expected: T selected; producer_of(x) is Transmission{T}; producer_of(y) is
  // AffineProjection sourced from x.position which itself resolves to a Transmission leaf
  // in one producer_of() step.
  const auto joints = ensure_joints(analysis_, {"j_a", "j_x", "j_y"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x_pos = ensure_state(analysis_, "j_x", InterfaceId{"position"});
  const auto y_pos = ensure_state(analysis_, "j_y", InterfaceId{"position"});

  // y mimics x with m=2, o=0.
  analysis_.add_affine_transmission(joints[1], joints[2], 2.0F, 0.0F);

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x_pos}, "T");

  TransmissionSubgraph subgraph(
    analysis_,
    std::vector<StateInterfaceId>{a},
    std::vector<StateInterfaceId>{x_pos, y_pos});

  EXPECT_TRUE(subgraph.is_complete());

  // x is produced by the transmission.
  const auto x_producer = as_transmission(subgraph.producer_of(x_pos));
  ASSERT_TRUE(x_producer.has_value());

  // y is produced by an AffineProjection sourced from x.
  const auto y_producer = as_affine(subgraph.producer_of(y_pos));
  ASSERT_TRUE(y_producer.has_value());
  EXPECT_EQ(y_producer->source, x_pos);
  EXPECT_TRUE(approx_equal(y_producer->multiplier, 2.0F));
  EXPECT_TRUE(approx_equal(y_producer->offset, 0.0F));

  // The AffineProjection's source resolves to a Transmission leaf in one producer_of() step
  // (verifies the leaf-source invariant: never another AffineProjection).
  ASSERT_TRUE(as_transmission(subgraph.producer_of(y_producer->source)).has_value());
}

// ===========================================================================
// add_input on a previously-ambiguous interface
// ===========================================================================

TEST_F(TransmissionSubgraphTest, AddInput_OnPreviouslyAmbiguousInterface_DiscardsAllCandidates)
{
  // Two transmissions T1: a → x and T2: a → x. x is ambiguous.
  // After add_input(x), x becomes Input and both candidates are recorded as discarded.
  ensure_joints(analysis_, {"j_a", "j_x"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T1");
  analysis_.add_transmission(model_id, {a}, {x}, "T2");

  TransmissionSubgraph subgraph(
    analysis_, std::vector<StateInterfaceId>{a}, std::vector<StateInterfaceId>{x});

  // Before: x is ambiguous.
  EXPECT_FALSE(subgraph.is_complete());
  EXPECT_TRUE(subgraph.is_ambiguous());
  ASSERT_EQ(subgraph.ambiguous_interfaces().size(), 1u);
  EXPECT_EQ(subgraph.ambiguous_interfaces()[0].interface, x);
  EXPECT_EQ(subgraph.ambiguous_interfaces()[0].candidates.size(), 2u);

  // Override x by adding it as an input.
  subgraph.add_input(x);

  // After: x is now Input, no longer ambiguous, plan is complete.
  EXPECT_TRUE(subgraph.is_complete());
  EXPECT_FALSE(subgraph.is_ambiguous());
  ASSERT_TRUE(as_input(subgraph.producer_of(x)).has_value());

  // input_overrides records the discard with both transmission candidates.
  ASSERT_EQ(subgraph.input_overrides().size(), 1u);
  const auto override_entry = subgraph.input_overrides()[0];
  EXPECT_EQ(override_entry.interface, x);
  ASSERT_EQ(override_entry.discarded_candidates.size(), 2u);
  // Both discarded candidates are Transmission producers.
  EXPECT_TRUE(as_transmission(override_entry.discarded_candidates[0]).has_value());
  EXPECT_TRUE(as_transmission(override_entry.discarded_candidates[1]).has_value());
}

// ===========================================================================
// Non-input ambiguity: Transmission vs AffineProjection
// ===========================================================================

TEST_F(TransmissionSubgraphTest, NonInputAmbiguity_TransmissionVsAffineProjection_IsReported)
{
  // Mimic A → B (B = 2A). Transmission T: x → B.position.
  // Inputs: {A.position, x}. Outputs: {B.position}.
  // B.position has two viable producers: AffineProjection from A.position and Transmission T.
  // The algorithm should refuse to pick a winner and report B.position as ambiguous.
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b", "j_x"});
  const auto a_pos = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b_pos = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});

  // B = 2A.
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0F, 0.0F);

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {x}, {b_pos}, "T");

  TransmissionSubgraph subgraph(
    analysis_,
    std::vector<StateInterfaceId>{a_pos, x},
    std::vector<StateInterfaceId>{b_pos});

  EXPECT_FALSE(subgraph.is_complete());
  EXPECT_TRUE(subgraph.is_ambiguous());
  ASSERT_EQ(subgraph.ambiguous_interfaces().size(), 1u);

  const auto amb = subgraph.ambiguous_interfaces()[0];
  EXPECT_EQ(amb.interface, b_pos);
  ASSERT_EQ(amb.candidates.size(), 2u);

  // One candidate should be a Transmission, the other an AffineProjection (in any order).
  bool saw_transmission = false;
  bool saw_affine = false;
  for (const auto & candidate : amb.candidates) {
    if (as_transmission(candidate).has_value()) saw_transmission = true;
    if (as_affine(candidate).has_value()) saw_affine = true;
  }
  EXPECT_TRUE(saw_transmission);
  EXPECT_TRUE(saw_affine);
}
