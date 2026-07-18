//
// Created by Bailey Chessum on 8/4/26.
//
// Unit tests for TransmissionReachability against synthetic TransmissionAnalysis instances.
// These tests cover the output-independent half of the analysis: which interfaces are derivable
// from given inputs, via which producer, with which ambiguities. Output-dependent concerns
// (unproducible, topological selection) live in test_joint_map_blueprint.cpp.
//

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <vector>

#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_model.hpp"
#include "arm_kinematics/joint_map/compute_transmission.hpp"
#include "arm_kinematics/joint_map/transmission_reachability.hpp"

using arm_kinematics::AffineProjectionRule;
using arm_kinematics::ComputeTransmission;
using arm_kinematics::InterfaceId;
using arm_kinematics::JointId;
using arm_kinematics::NamedStateInterfaceDefinition;
using arm_kinematics::StateInterfaceDefinition;
using arm_kinematics::TransmissionAnalysis;
using arm_kinematics::TransmissionInstanceId;
using arm_kinematics::TransmissionModel;
using arm_kinematics::TransmissionReachability;
using StateInterfaceId = TransmissionAnalysis::StateInterfaceId;

namespace producers = arm_kinematics::producers;
using producers::StateInterfaceProducer;

namespace {

// ---------------------------------------------------------------------------
// Test mocks
// ---------------------------------------------------------------------------

class StubComputeTransmission : public ComputeTransmission {
public:
  void compute(
    arm_kinematics::span<const double>,
    arm_kinematics::span<double>,
    arm_kinematics::span<double>) const override
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

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

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

StateInterfaceDefinition def_of(const TransmissionAnalysis & analysis, const StateInterfaceId sid)
{
  return analysis.state_interface_order().inverse[sid];
}

// Producer-variant unwrappers — return std::optional<T> (a copy) so callers don't risk a
// dangling pointer into a temporary StateInterfaceProducer returned by reach.producer_of().
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

constexpr double kTolerance = 1.0e-9;

bool approx_equal(double a, double b)
{
  return std::abs(a - b) <= kTolerance + kTolerance * std::max(std::abs(a), std::abs(b));
}

}  // namespace

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class TransmissionReachabilityTest : public ::testing::Test
{
protected:
  TransmissionAnalysis analysis_{};
};

// ===========================================================================
// Pure transmission graph
// ===========================================================================

TEST_F(TransmissionReachabilityTest, PureTransmission_SingleProducer)
{
  // a, b → T → c
  ensure_joints(analysis_, {"j_a", "j_b", "j_c"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis_, "j_c", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a, b}, {c}, "T");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a), def_of(analysis_, b)});

  EXPECT_FALSE(reach.is_ambiguous());
  EXPECT_TRUE(reach.ambiguities().empty());

  ASSERT_TRUE(as_input(reach.producer_of(a)).has_value());
  ASSERT_TRUE(as_input(reach.producer_of(b)).has_value());
  EXPECT_EQ(as_input(reach.producer_of(a))->input_index, 0u);
  EXPECT_EQ(as_input(reach.producer_of(b))->input_index, 1u);

  const auto c_producer = as_transmission(reach.producer_of(c));
  ASSERT_TRUE(c_producer.has_value());
  EXPECT_EQ(c_producer->instance_id, 0u);
}

TEST_F(TransmissionReachabilityTest, PureTransmission_MissingInput_OutputIsMonostate)
{
  // a, b → T → c. Only `a` is supplied, so c is not derivable.
  ensure_joints(analysis_, {"j_a", "j_b", "j_c"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis_, "j_c", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a, b}, {c}, "T");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a)});

  EXPECT_FALSE(reach.is_ambiguous());
  EXPECT_TRUE(is_monostate(reach.producer_of(c)));
  EXPECT_TRUE(is_monostate(reach.producer_of(b)));
}

// ===========================================================================
// Affine projection — three rule types
// ===========================================================================

TEST_F(TransmissionReachabilityTest, AffineProjection_PositionRule_Composition)
{
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b"});
  const auto a_pos = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b_pos = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0, 5.0);

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a_pos)});

  const auto proj = as_affine(reach.producer_of(b_pos));
  ASSERT_TRUE(proj.has_value());
  EXPECT_EQ(proj->source, def_of(analysis_, a_pos));
  EXPECT_TRUE(approx_equal(proj->multiplier, 2.0));
  EXPECT_TRUE(approx_equal(proj->offset, 5.0));
}

TEST_F(TransmissionReachabilityTest, AffineProjection_VelocityRule_DropsOffset)
{
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b"});
  const auto a_vel = ensure_state(analysis_, "j_a", InterfaceId{"velocity"});
  const auto b_vel = ensure_state(analysis_, "j_b", InterfaceId{"velocity"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0, 5.0);

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a_vel)});

  const auto proj = as_affine(reach.producer_of(b_vel));
  ASSERT_TRUE(proj.has_value());
  EXPECT_EQ(proj->source, def_of(analysis_, a_vel));
  EXPECT_TRUE(approx_equal(proj->multiplier, 2.0));
  EXPECT_TRUE(approx_equal(proj->offset, 0.0));
}

TEST_F(TransmissionReachabilityTest, AffineProjection_EffortRule_RequiresExplicitRegistration)
{
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b"});
  const auto a_eff = ensure_state(analysis_, "j_a", InterfaceId{"effort"});
  const auto b_eff = ensure_state(analysis_, "j_b", InterfaceId{"effort"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0, 5.0);

  // Without registering an effort rule, B.effort is monostate from A.effort.
  {
    const auto reach = TransmissionReachability::analyze(
      analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a_eff)});
    EXPECT_TRUE(is_monostate(reach.producer_of(b_eff)));
  }

  // Register the effort rule explicitly.
  analysis_.set_affine_projection_rule(
    InterfaceId{"effort"},
    AffineProjectionRule{1.0, 0.0, true});

  {
    const auto reach = TransmissionReachability::analyze(
      analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a_eff)});
    const auto proj = as_affine(reach.producer_of(b_eff));
    ASSERT_TRUE(proj.has_value());
    EXPECT_EQ(proj->source, def_of(analysis_, a_eff));
    EXPECT_TRUE(approx_equal(proj->multiplier, 0.5));
    EXPECT_TRUE(approx_equal(proj->offset, 0.0));
  }
}

// ===========================================================================
// Affine chain composition
// ===========================================================================

TEST_F(TransmissionReachabilityTest, AffineChain_ComposesAcrossThreeJoints)
{
  // A → B → C with B = 2A + 5, C = 3B + 7.  Composed: C = 6A + 22.
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b", "j_c"});
  const auto a_pos = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b_pos = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto c_pos = ensure_state(analysis_, "j_c", InterfaceId{"position"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0, 5.0);
  analysis_.add_affine_transmission(joints[1], joints[2], 3.0, 7.0);

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a_pos)});

  const auto b_proj = as_affine(reach.producer_of(b_pos));
  ASSERT_TRUE(b_proj.has_value());
  EXPECT_TRUE(approx_equal(b_proj->multiplier, 2.0));
  EXPECT_TRUE(approx_equal(b_proj->offset, 5.0));

  const auto c_proj = as_affine(reach.producer_of(c_pos));
  ASSERT_TRUE(c_proj.has_value());
  EXPECT_TRUE(approx_equal(c_proj->multiplier, 6.0));
  EXPECT_TRUE(approx_equal(c_proj->offset, 22.0));
}

// ===========================================================================
// No projection rule = no propagation
// ===========================================================================

TEST_F(TransmissionReachabilityTest, AffineProjection_NoRuleForInterface_DoesNotPropagate)
{
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b"});
  const auto a_custom = ensure_state(analysis_, "j_a", InterfaceId{"custom_iface"});
  const auto b_custom = ensure_state(analysis_, "j_b", InterfaceId{"custom_iface"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0, 0.0);

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a_custom)});

  EXPECT_TRUE(is_monostate(reach.producer_of(b_custom)));
}

// ===========================================================================
// Viability filtering
// ===========================================================================

TEST_F(TransmissionReachabilityTest, ViabilityFiltering_TransmissionWithUnreachableInputsIsNotACandidate)
{
  // T1: a → x  (a supplied)
  // T2: b → x  (b NOT supplied)
  // x has only T1 as a candidate.
  ensure_joints(analysis_, {"j_a", "j_b", "j_x"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T1");
  analysis_.add_transmission(model_id, {b}, {x}, "T2");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a)});

  EXPECT_FALSE(reach.is_ambiguous());
  const auto x_producer = as_transmission(reach.producer_of(x));
  ASSERT_TRUE(x_producer.has_value());
  EXPECT_EQ(x_producer->instance_id, 0u);  // T1
}

// ===========================================================================
// Re-analyze with augmented inputs (replaces add_input mutation API)
// ===========================================================================

TEST_F(TransmissionReachabilityTest, ReAnalyzeWithAugmentedInputs_UnblocksMissingInterface)
{
  // T: a, b → c. Initial reachability lacks b. Augmented one supplies it.
  ensure_joints(analysis_, {"j_a", "j_b", "j_c"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis_, "j_c", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a, b}, {c}, "T");

  const auto reach1 = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a)});
  EXPECT_TRUE(is_monostate(reach1.producer_of(c)));

  const auto reach2 = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a), def_of(analysis_, b)});
  ASSERT_TRUE(as_transmission(reach2.producer_of(c)).has_value());
}

TEST_F(TransmissionReachabilityTest, ReAnalyzeWithExplicitOverride_TransmissionDisplacedByInput)
{
  // T1: a → x. In reach1, x's producer is T1. In reach2, x is supplied directly → x's
  // producer is Input (input wins).
  ensure_joints(analysis_, {"j_a", "j_x"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T1");

  const auto reach1 = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a)});
  ASSERT_TRUE(as_transmission(reach1.producer_of(x)).has_value());

  const auto reach2 = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a), def_of(analysis_, x)});
  ASSERT_TRUE(as_input(reach2.producer_of(x)).has_value());
}

TEST_F(TransmissionReachabilityTest, ReAnalyzeWithExplicitInput_DisplacesPreviouslyAmbiguous)
{
  // Two transmissions both produce x → ambiguous. Then supply x directly → unambiguous Input.
  ensure_joints(analysis_, {"j_a", "j_x"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T1");
  analysis_.add_transmission(model_id, {a}, {x}, "T2");

  const auto reach1 = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a)});
  EXPECT_TRUE(reach1.is_ambiguous());
  ASSERT_EQ(reach1.ambiguities().size(), 1u);
  EXPECT_EQ(reach1.ambiguities()[0].interface, def_of(analysis_, x));
  EXPECT_EQ(reach1.ambiguities()[0].candidates.size(), 2u);

  const auto reach2 = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a), def_of(analysis_, x)});
  EXPECT_FALSE(reach2.is_ambiguous());
  ASSERT_TRUE(as_input(reach2.producer_of(x)).has_value());
}

// ===========================================================================
// Duplicate inputs
// ===========================================================================

TEST_F(TransmissionReachabilityTest, DuplicateInterfaceInInputs_FirstOccurrenceWins)
{
  ensure_joints(analysis_, {"j_a"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a), def_of(analysis_, a)});

  ASSERT_EQ(reach.inputs().size(), 2u);
  EXPECT_EQ(reach.inputs()[0], def_of(analysis_, a));
  EXPECT_EQ(reach.inputs()[1], def_of(analysis_, a));
  EXPECT_EQ(as_input(reach.producer_of(a))->input_index, 0u);
}

// ===========================================================================
// Input wins
// ===========================================================================

TEST_F(TransmissionReachabilityTest, InputWinsOverDerivedAffineProducer)
{
  // B mimics A. User supplies BOTH A.position AND B.position. Both Input, no ambiguity.
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b"});
  const auto a_pos = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b_pos = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0, 5.0);

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a_pos), def_of(analysis_, b_pos)});

  EXPECT_FALSE(reach.is_ambiguous());
  ASSERT_TRUE(as_input(reach.producer_of(a_pos)).has_value());
  ASSERT_TRUE(as_input(reach.producer_of(b_pos)).has_value());
}

TEST_F(TransmissionReachabilityTest, InputWinsOverTransmission)
{
  // T: a → x. User supplies x as input directly. x's producer is Input.
  ensure_joints(analysis_, {"j_a", "j_x"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a), def_of(analysis_, x)});
  ASSERT_TRUE(as_input(reach.producer_of(x)).has_value());
}

// ===========================================================================
// Ambiguity
// ===========================================================================

TEST_F(TransmissionReachabilityTest, AmbiguityAccumulation_MultipleAmbiguousInterfacesAllReported)
{
  ensure_joints(analysis_, {"j_a", "j_x", "j_y"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});
  const auto y = ensure_state(analysis_, "j_y", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T1");
  analysis_.add_transmission(model_id, {a}, {x}, "T2");
  analysis_.add_transmission(model_id, {a}, {y}, "T3");
  analysis_.add_transmission(model_id, {a}, {y}, "T4");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a)});

  EXPECT_TRUE(reach.is_ambiguous());
  ASSERT_EQ(reach.ambiguities().size(), 2u);
  for (const auto & ai : reach.ambiguities()) {
    EXPECT_EQ(ai.candidates.size(), 2u);
    EXPECT_TRUE(ai.interface == def_of(analysis_, x) || ai.interface == def_of(analysis_, y));
  }
  EXPECT_TRUE(is_monostate(reach.producer_of(x)));
  EXPECT_TRUE(is_monostate(reach.producer_of(y)));
}

TEST_F(TransmissionReachabilityTest, NonAmbiguousInterfacesInOtherwiseAmbiguousReach_StillReturnTheirProducer)
{
  ensure_joints(analysis_, {"j_a", "j_x", "j_z"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});
  const auto z = ensure_state(analysis_, "j_z", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T1");
  analysis_.add_transmission(model_id, {a}, {x}, "T2");
  analysis_.add_transmission(model_id, {a}, {z}, "T3");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a)});

  EXPECT_TRUE(reach.is_ambiguous());
  EXPECT_TRUE(is_monostate(reach.producer_of(x)));
  ASSERT_TRUE(as_transmission(reach.producer_of(z)).has_value());
}

// ===========================================================================
// Mixed-quantity transmission
// ===========================================================================

TEST_F(TransmissionReachabilityTest, MixedQuantityTransmission_PositionInEffortOut)
{
  ensure_joints(analysis_, {"j_a", "j_x"});
  const auto a_pos = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x_eff = ensure_state(analysis_, "j_x", InterfaceId{"effort"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a_pos}, {x_eff}, "T");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a_pos)});

  ASSERT_TRUE(as_transmission(reach.producer_of(x_eff)).has_value());
}

// ===========================================================================
// Side-effect outputs
// ===========================================================================

TEST_F(TransmissionReachabilityTest, UserSuppliedSideEffectOutput_InputWinsOverTransmissionSideEffect)
{
  // T produces both K and I from {a, b}. User supplies a, b, I. Reachability shows I as Input
  // (input wins over T's side-effect production), K as Transmission.
  ensure_joints(analysis_, {"j_a", "j_b", "j_i", "j_k"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto i = ensure_state(analysis_, "j_i", InterfaceId{"position"});
  const auto k = ensure_state(analysis_, "j_k", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a, b}, {i, k}, "T");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a), def_of(analysis_, b), def_of(analysis_, i)});

  ASSERT_TRUE(as_input(reach.producer_of(i)).has_value());
  ASSERT_TRUE(as_transmission(reach.producer_of(k)).has_value());
}

// ===========================================================================
// Redundant equivalent inputs
// ===========================================================================

TEST_F(TransmissionReachabilityTest, RedundantEquivalentInputs_MultipleLeavesInSameAffineGroup)
{
  // A → B (B = 2A), A → D (D = 3A). User supplies A and D. The lowest JointId leaf (A) wins
  // as the affine source for B; D becomes a redundant equivalent input.
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b", "j_d"});
  const auto a_pos = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b_pos = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto d_pos = ensure_state(analysis_, "j_d", InterfaceId{"position"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0, 0.0);
  analysis_.add_affine_transmission(joints[0], joints[2], 3.0, 0.0);

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a_pos), def_of(analysis_, d_pos)});

  const auto b_proj = as_affine(reach.producer_of(b_pos));
  ASSERT_TRUE(b_proj.has_value());
  EXPECT_EQ(b_proj->source, def_of(analysis_, a_pos));
  ASSERT_EQ(reach.redundant_equivalent_inputs().size(), 1u);
  EXPECT_EQ(reach.redundant_equivalent_inputs()[0], def_of(analysis_, d_pos));
  ASSERT_TRUE(as_input(reach.producer_of(d_pos)).has_value());
}

// ===========================================================================
// Accessor tests
// ===========================================================================

TEST_F(TransmissionReachabilityTest, AnalysisAccessor_ReturnsTheSameAnalysis)
{
  ensure_joints(analysis_, {"j_a"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a)});
  EXPECT_EQ(&reach.analysis(), &analysis_);
}

TEST_F(TransmissionReachabilityTest, DerivableInterfaces_IncludesInputsAndDerivedOutputs)
{
  // a → T → c. derivable should contain {a, c}.
  ensure_joints(analysis_, {"j_a", "j_c"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto c = ensure_state(analysis_, "j_c", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {c}, "T");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a)});

  const auto derivable = reach.derivable_interfaces();
  ASSERT_EQ(derivable.size(), 2u);
  EXPECT_EQ(derivable[0], a);
  EXPECT_EQ(derivable[1], c);
}

// ===========================================================================
// Affine projection sourced from a transmission output (leaf-source invariant)
// ===========================================================================

TEST_F(TransmissionReachabilityTest, AffineProjection_SourcedFromTransmissionOutput_ResolvesToLeaf)
{
  // T: a → x.position. Mimic edge x → y. y mimics x with multiplier 2.
  // Expected: producer_of(x) is Transmission{T}; producer_of(y) is AffineProjection sourced
  // from x.position which itself resolves to a Transmission leaf in one step.
  const auto joints = ensure_joints(analysis_, {"j_a", "j_x", "j_y"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x_pos = ensure_state(analysis_, "j_x", InterfaceId{"position"});
  const auto y_pos = ensure_state(analysis_, "j_y", InterfaceId{"position"});

  analysis_.add_affine_transmission(joints[1], joints[2], 2.0, 0.0);

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x_pos}, "T");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a)});

  const auto x_producer = as_transmission(reach.producer_of(x_pos));
  ASSERT_TRUE(x_producer.has_value());

  const auto y_producer = as_affine(reach.producer_of(y_pos));
  ASSERT_TRUE(y_producer.has_value());
  EXPECT_EQ(y_producer->source, def_of(analysis_, x_pos));
  EXPECT_TRUE(approx_equal(y_producer->multiplier, 2.0));
  EXPECT_TRUE(approx_equal(y_producer->offset, 0.0));

  // Source resolves to a Transmission leaf in one step.
  ASSERT_TRUE(as_transmission(reach.producer_of_def(y_producer->source)).has_value());
}

// ===========================================================================
// Non-input ambiguity: Transmission vs AffineProjection
// ===========================================================================

TEST_F(TransmissionReachabilityTest, NonInputAmbiguity_TransmissionVsAffineProjection_IsReported)
{
  // Mimic A → B (B = 2A). Transmission T: x → B.position. Inputs: {A.position, x}.
  // B.position has both an AffineProjection from A and a Transmission from x → ambiguous.
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b", "j_x"});
  const auto a_pos = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b_pos = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});

  analysis_.add_affine_transmission(joints[0], joints[1], 2.0, 0.0);

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {x}, {b_pos}, "T");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a_pos), def_of(analysis_, x)});

  EXPECT_TRUE(reach.is_ambiguous());
  ASSERT_EQ(reach.ambiguities().size(), 1u);

  const auto amb = reach.ambiguities()[0];
  EXPECT_EQ(amb.interface, def_of(analysis_, b_pos));
  ASSERT_EQ(amb.candidates.size(), 2u);

  bool saw_transmission = false;
  bool saw_affine = false;
  for (const auto & candidate : amb.candidates) {
    if (as_transmission(candidate).has_value()) saw_transmission = true;
    if (as_affine(candidate).has_value()) saw_affine = true;
  }
  EXPECT_TRUE(saw_transmission);
  EXPECT_TRUE(saw_affine);
}

// ===========================================================================
// Algorithm fix: ambiguity does NOT propagate through downstream transmissions
// ===========================================================================

TEST_F(TransmissionReachabilityTest, Ambiguity_DoesNotPropagateThroughChain)
{
  // T1: a → x;  T2: a → x   (x is ambiguous)
  // T3: x → y                  (y depends on x; the algorithm fix means y is BLOCKED, not
  //                              derivable, and not ambiguous either — it has no producer at
  //                              all because T3 is skipped during the inner loop)
  ensure_joints(analysis_, {"j_a", "j_x", "j_y"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});
  const auto y = ensure_state(analysis_, "j_y", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T1");
  analysis_.add_transmission(model_id, {a}, {x}, "T2");
  analysis_.add_transmission(model_id, {x}, {y}, "T3");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a)});

  // x is the directly-ambiguous interface.
  EXPECT_TRUE(reach.is_ambiguous());
  ASSERT_EQ(reach.ambiguities().size(), 1u);
  EXPECT_EQ(reach.ambiguities()[0].interface, def_of(analysis_, x));

  // y has NO producer — not derivable (T3 was skipped because x is ambiguous), not ambiguous
  // (it has no candidates because T3 didn't fire).
  EXPECT_TRUE(is_monostate(reach.producer_of(y)));

  // Derivable contains only `a` — neither x nor y propagated.
  const auto derivable = reach.derivable_interfaces();
  EXPECT_EQ(derivable.size(), 1u);
  EXPECT_EQ(derivable[0], a);
}

TEST_F(TransmissionReachabilityTest, Blocked_OutputDownstreamOfAmbiguous_IsBlocked_NotDerivable)
{
  // Same setup as above, but assert via the transitively_blocked_interfaces() query.
  ensure_joints(analysis_, {"j_a", "j_x", "j_y"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  ensure_state(analysis_, "j_x", InterfaceId{"position"});
  const auto y = ensure_state(analysis_, "j_y", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  const auto x_id = ensure_state(analysis_, "j_x", InterfaceId{"position"});
  analysis_.add_transmission(model_id, {a}, {x_id}, "T1");
  analysis_.add_transmission(model_id, {a}, {x_id}, "T2");
  analysis_.add_transmission(model_id, {x_id}, {y}, "T3");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a)});

  // y is in transitively_blocked_interfaces() — its producer chain transitively touches an ambiguous
  // interface (x).
  const auto blocked = reach.transitively_blocked_interfaces();
  ASSERT_EQ(blocked.size(), 1u);
  EXPECT_EQ(blocked[0], y);
}

TEST_F(TransmissionReachabilityTest, Blocked_CascadeThroughTwoLayers)
{
  // T1, T2: a → x   (x ambiguous)
  // T3: x → y       (y blocked by x)
  // T4: y → z       (z blocked by y, transitively by x)
  ensure_joints(analysis_, {"j_a", "j_x", "j_y", "j_z"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});
  const auto y = ensure_state(analysis_, "j_y", InterfaceId{"position"});
  const auto z = ensure_state(analysis_, "j_z", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T1");
  analysis_.add_transmission(model_id, {a}, {x}, "T2");
  analysis_.add_transmission(model_id, {x}, {y}, "T3");
  analysis_.add_transmission(model_id, {y}, {z}, "T4");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a)});

  ASSERT_EQ(reach.ambiguities().size(), 1u);
  EXPECT_EQ(reach.ambiguities()[0].interface, def_of(analysis_, x));

  // Both y and z are blocked.
  const auto blocked = reach.transitively_blocked_interfaces();
  ASSERT_EQ(blocked.size(), 2u);
  // Order may vary; check via membership.
  bool saw_y = false, saw_z = false;
  for (const auto sid : blocked) {
    if (sid == y) saw_y = true;
    if (sid == z) saw_z = true;
  }
  EXPECT_TRUE(saw_y);
  EXPECT_TRUE(saw_z);
}

// ===========================================================================
// Second-order ambiguity: x has 2 producers, but one of them is upstream-blocked
// ===========================================================================

TEST_F(TransmissionReachabilityTest, Ambiguity_DownstreamProducerWithCleanAlternativeStaysAmbiguous)
{
  // Setup:
  //   T1: a → x   (clean — input a is supplied directly)
  //   T2: b → x   (T2's input b will become ambiguous)
  //   T3: c → b
  //   T4: c → b   (b ambiguous via T3 and T4)
  // Inputs: {a, c}
  //
  // The user wrote BOTH T1 and T2 as producers for x. After b is recognized as ambiguous, T2
  // can't actually fire — but the user's authorial intent was "x has multiple producers".
  // The algorithm should report x as ambiguous (with snapshot [T1, T2]) NOT as derivable via
  // T1, because the user needs to know they have a structural ambiguity in their analysis.
  //
  // This test would have FAILED on the previous code's revocation logic, which committed
  // x = T1 in pass 2 (when only T1 actually fires), leaving an inconsistent four-way
  // contradiction (`producer_of(x)` returned T1 while x was simultaneously in
  // `transitively_blocked_interfaces_()` and absent from `derivable_interfaces_()`).
  ensure_joints(analysis_, {"j_a", "j_b", "j_c", "j_x"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis_, "j_c", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a}, {x}, "T1");
  analysis_.add_transmission(model_id, {b}, {x}, "T2");
  analysis_.add_transmission(model_id, {c}, {b}, "T3");
  analysis_.add_transmission(model_id, {c}, {b}, "T4");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, a), def_of(analysis_, c)});

  // Both x and b are ambiguous.
  EXPECT_TRUE(reach.is_ambiguous());
  ASSERT_EQ(reach.ambiguities().size(), 2u);

  // Find the AmbiguousInterface entry for x and verify it has BOTH T1 and T2 in the snapshot.
  bool found_x_ambig = false;
  bool found_b_ambig = false;
  for (const auto & amb : reach.ambiguities()) {
    if (amb.interface == def_of(analysis_, x)) {
      found_x_ambig = true;
      ASSERT_EQ(amb.candidates.size(), 2u);
      // Both candidates should be Transmissions.
      bool saw_both_transmissions = true;
      for (const auto & c : amb.candidates) {
        if (!as_transmission(c).has_value()) saw_both_transmissions = false;
      }
      EXPECT_TRUE(saw_both_transmissions);
    }
    if (amb.interface == def_of(analysis_, b)) {
      found_b_ambig = true;
      ASSERT_EQ(amb.candidates.size(), 2u);
    }
  }
  EXPECT_TRUE(found_x_ambig);
  EXPECT_TRUE(found_b_ambig);

  // x's producer is monostate (NOT T1).
  EXPECT_TRUE(is_monostate(reach.producer_of(x)));

  // x is NOT in derivable_interfaces_.
  bool x_in_derivable = false;
  for (const auto sid : reach.derivable_interfaces()) {
    if (sid == x) x_in_derivable = true;
  }
  EXPECT_FALSE(x_in_derivable);

  // x is NOT in transitively_blocked_interfaces_ either (it's ambiguous, not blocked).
  bool x_in_blocked = false;
  for (const auto sid : reach.transitively_blocked_interfaces()) {
    if (sid == x) x_in_blocked = true;
  }
  EXPECT_FALSE(x_in_blocked);
}

// ===========================================================================
// Forward/inverse transmission pairs: the analysis is not a DAG and that's OK
// ===========================================================================

TEST_F(TransmissionReachabilityTest, ForwardInversePair_MotorInput_OnlyForwardSelected)
{
  // T_fwd: motor → joint
  // T_inv: joint → motor
  // Inputs: {motor}
  // Expected: motor is Input, joint is derivable via T_fwd. T_inv fires harmlessly in the
  // inner loop but its output is input-won and not selected by plan_joint_map.
  ensure_joints(analysis_, {"j_motor", "j_joint"});
  const auto motor = ensure_state(analysis_, "j_motor", InterfaceId{"position"});
  const auto joint = ensure_state(analysis_, "j_joint", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {motor}, {joint}, "T_fwd");
  analysis_.add_transmission(model_id, {joint}, {motor}, "T_inv");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, motor)});

  EXPECT_FALSE(reach.is_ambiguous());
  ASSERT_TRUE(as_input(reach.producer_of(motor)).has_value());
  ASSERT_TRUE(as_transmission(reach.producer_of(joint)).has_value());
  EXPECT_TRUE(reach.transitively_blocked_interfaces().empty());
}

TEST_F(TransmissionReachabilityTest, ForwardInversePair_JointInput_OnlyInverseSelected)
{
  ensure_joints(analysis_, {"j_motor", "j_joint"});
  const auto motor = ensure_state(analysis_, "j_motor", InterfaceId{"position"});
  const auto joint = ensure_state(analysis_, "j_joint", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {motor}, {joint}, "T_fwd");
  analysis_.add_transmission(model_id, {joint}, {motor}, "T_inv");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, joint)});

  EXPECT_FALSE(reach.is_ambiguous());
  ASSERT_TRUE(as_input(reach.producer_of(joint)).has_value());
  ASSERT_TRUE(as_transmission(reach.producer_of(motor)).has_value());
}

TEST_F(TransmissionReachabilityTest, ForwardInversePair_BothInputs_BothInputWins)
{
  ensure_joints(analysis_, {"j_motor", "j_joint"});
  const auto motor = ensure_state(analysis_, "j_motor", InterfaceId{"position"});
  const auto joint = ensure_state(analysis_, "j_joint", InterfaceId{"position"});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {motor}, {joint}, "T_fwd");
  analysis_.add_transmission(model_id, {joint}, {motor}, "T_inv");

  const auto reach = TransmissionReachability::analyze(
    analysis_, std::vector<StateInterfaceDefinition>{def_of(analysis_, motor), def_of(analysis_, joint)});

  EXPECT_FALSE(reach.is_ambiguous());
  // Both should resolve to Input (input wins on both sides).
  ASSERT_TRUE(as_input(reach.producer_of(motor)).has_value());
  ASSERT_TRUE(as_input(reach.producer_of(joint)).has_value());
}
