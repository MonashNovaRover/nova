//
// Created by Bailey Chessum on 8/4/26.
//
// Direct unit tests for TransmissionAnalysis, independent of TransmissionReachability.
// Focus is on the per-joint flat affine relations, the affine group union-find, the
// inverse producing-transmissions index, and the projection rule registry.
//

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>

#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_model.hpp"
#include "arm_kinematics/joint_map/compute_transmission.hpp"

using arm_kinematics::AffineProjectionRule;
using arm_kinematics::ComputeTransmission;
using arm_kinematics::InterfaceId;
using arm_kinematics::JointId;
using arm_kinematics::NamedStateInterfaceDefinition;
using arm_kinematics::StateInterfaceId;
using arm_kinematics::TransmissionAnalysis;
using arm_kinematics::TransmissionInstanceId;
using arm_kinematics::TransmissionModel;

namespace {

// Same trivial stubs as the subgraph tests — TransmissionAnalysis only stores TransmissionModels
// and never invokes them, so the bodies are unreachable.
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

constexpr double kTolerance = 1.0e-9;

bool approx_equal(double a, double b)
{
  return std::fabs(a - b) <= kTolerance + kTolerance * std::max(std::fabs(a), std::fabs(b));
}

}  // namespace

class TransmissionAnalysisTest : public ::testing::Test
{
protected:
  TransmissionAnalysis analysis_{};
};

// ===========================================================================
// Per-joint flat affine relations (identity + composition)
// ===========================================================================

TEST_F(TransmissionAnalysisTest, IsolatedJoint_AffineRootIsItself_GroupIsSingleton_FlatIsIdentity)
{
  const auto j = analysis_.ensure_joint_id("j");

  EXPECT_EQ(analysis_.affine_root_of(j), j);

  const auto members = analysis_.affine_group_members(j);
  ASSERT_EQ(members.size(), 1u);
  EXPECT_EQ(members[0], j);

  const auto & flat = analysis_.affine_transmission_of(j);
  EXPECT_EQ(flat.source_joint_id, j);
  EXPECT_EQ(flat.target_joint_id, j);
  EXPECT_TRUE(approx_equal(flat.multiplier, 1.0));
  EXPECT_TRUE(approx_equal(flat.offset, 0.0));
}

TEST_F(TransmissionAnalysisTest, MimicChain_ProducesCorrectFlatComposition)
{
  // A → B (m=2, o=5), B → C (m=3, o=7).
  // Composed: B = 2A + 5; C = 3B + 7 = 3(2A + 5) + 7 = 6A + 22.
  const auto a = analysis_.ensure_joint_id("a");
  const auto b = analysis_.ensure_joint_id("b");
  const auto c = analysis_.ensure_joint_id("c");

  analysis_.add_affine_transmission(a, b, 2.0, 5.0);
  analysis_.add_affine_transmission(b, c, 3.0, 7.0);

  EXPECT_EQ(analysis_.affine_root_of(a), a);
  EXPECT_EQ(analysis_.affine_root_of(b), a);
  EXPECT_EQ(analysis_.affine_root_of(c), a);

  const auto & flat_c = analysis_.affine_transmission_of(c);
  EXPECT_EQ(flat_c.source_joint_id, a);
  EXPECT_EQ(flat_c.target_joint_id, c);
  EXPECT_TRUE(approx_equal(flat_c.multiplier, 6.0));
  EXPECT_TRUE(approx_equal(flat_c.offset, 22.0));

  const auto & flat_b = analysis_.affine_transmission_of(b);
  EXPECT_EQ(flat_b.source_joint_id, a);
  EXPECT_TRUE(approx_equal(flat_b.multiplier, 2.0));
  EXPECT_TRUE(approx_equal(flat_b.offset, 5.0));
}

TEST_F(TransmissionAnalysisTest, ChainCompositionIsOrderIndependent)
{
  // Build A→B (m=2, o=5), B→C (m=3, o=7) two ways and verify the final flats agree.
  TransmissionAnalysis a1{};
  const auto a1_a = a1.ensure_joint_id("a");
  const auto a1_b = a1.ensure_joint_id("b");
  const auto a1_c = a1.ensure_joint_id("c");
  a1.add_affine_transmission(a1_a, a1_b, 2.0, 5.0);
  a1.add_affine_transmission(a1_b, a1_c, 3.0, 7.0);

  TransmissionAnalysis a2{};
  const auto a2_a = a2.ensure_joint_id("a");
  const auto a2_b = a2.ensure_joint_id("b");
  const auto a2_c = a2.ensure_joint_id("c");
  a2.add_affine_transmission(a2_b, a2_c, 3.0, 7.0);
  a2.add_affine_transmission(a2_a, a2_b, 2.0, 5.0);

  const auto & f1 = a1.affine_transmission_of(a1_c);
  const auto & f2 = a2.affine_transmission_of(a2_c);
  EXPECT_TRUE(approx_equal(f1.multiplier, f2.multiplier));
  EXPECT_TRUE(approx_equal(f1.offset, f2.offset));
  EXPECT_EQ(a1.affine_root_of(a1_c), a1_a);
  EXPECT_EQ(a2.affine_root_of(a2_c), a2_a);
}

TEST_F(TransmissionAnalysisTest, MergingNonTrivialGroupsViaNonRootEdge)
{
  // Group 1: {A, B} via B mimics A (A→B, m=2, o=5)
  // Group 2: {C, D} via D mimics C (C→D, m=4, o=1)
  // Then add B→C (m=3, o=2) — non-root edge merging two non-trivial groups.
  // After merge: A is root of all four. Verify each one's composed flat.
  const auto a = analysis_.ensure_joint_id("a");
  const auto b = analysis_.ensure_joint_id("b");
  const auto c = analysis_.ensure_joint_id("c");
  const auto d = analysis_.ensure_joint_id("d");

  analysis_.add_affine_transmission(a, b, 2.0, 5.0);   // B = 2A + 5
  analysis_.add_affine_transmission(c, d, 4.0, 1.0);   // D = 4C + 1
  analysis_.add_affine_transmission(b, c, 3.0, 2.0);   // C = 3B + 2

  EXPECT_EQ(analysis_.affine_root_of(a), a);
  EXPECT_EQ(analysis_.affine_root_of(b), a);
  EXPECT_EQ(analysis_.affine_root_of(c), a);
  EXPECT_EQ(analysis_.affine_root_of(d), a);

  // C in terms of A: C = 3B + 2 = 3(2A + 5) + 2 = 6A + 17.
  const auto & flat_c = analysis_.affine_transmission_of(c);
  EXPECT_TRUE(approx_equal(flat_c.multiplier, 6.0));
  EXPECT_TRUE(approx_equal(flat_c.offset, 17.0));

  // D in terms of A: D = 4C + 1 = 4(6A + 17) + 1 = 24A + 69.
  const auto & flat_d = analysis_.affine_transmission_of(d);
  EXPECT_TRUE(approx_equal(flat_d.multiplier, 24.0));
  EXPECT_TRUE(approx_equal(flat_d.offset, 69.0));

  // The group has all four members.
  const auto members = analysis_.affine_group_members(a);
  EXPECT_EQ(members.size(), 4u);
}

// ===========================================================================
// add_affine_transmission preconditions
// ===========================================================================

TEST_F(TransmissionAnalysisTest, MultiplierZero_IsRejected)
{
  const auto a = analysis_.ensure_joint_id("a");
  const auto b = analysis_.ensure_joint_id("b");
  EXPECT_THROW(
    analysis_.add_affine_transmission(a, b, 0.0, 5.0),
    std::invalid_argument);
}

TEST_F(TransmissionAnalysisTest, SelfLoop_IsRejected)
{
  const auto a = analysis_.ensure_joint_id("a");
  EXPECT_THROW(
    analysis_.add_affine_transmission(a, a, 2.0, 0.0),
    std::invalid_argument);
}

TEST_F(TransmissionAnalysisTest, RedundantConsistentEdge_IsNoOp)
{
  const auto a = analysis_.ensure_joint_id("a");
  const auto b = analysis_.ensure_joint_id("b");
  analysis_.add_affine_transmission(a, b, 2.0, 5.0);

  // Re-adding the same edge with the same coefficients must not throw and must leave state
  // unchanged.
  EXPECT_NO_THROW(analysis_.add_affine_transmission(a, b, 2.0, 5.0));

  EXPECT_EQ(analysis_.affine_root_of(b), a);
  const auto & flat = analysis_.affine_transmission_of(b);
  EXPECT_TRUE(approx_equal(flat.multiplier, 2.0));
  EXPECT_TRUE(approx_equal(flat.offset, 5.0));
  EXPECT_EQ(analysis_.affine_group_members(a).size(), 2u);
}

// ===========================================================================
// Joint id assignment
// ===========================================================================

TEST_F(TransmissionAnalysisTest, EnsureJointId_AssignsConsecutiveIds_FromZero)
{
  const auto a = analysis_.ensure_joint_id("a");
  const auto b = analysis_.ensure_joint_id("b");
  const auto c = analysis_.ensure_joint_id("c");
  EXPECT_EQ(a, static_cast<JointId>(0));
  EXPECT_EQ(b, static_cast<JointId>(1));
  EXPECT_EQ(c, static_cast<JointId>(2));

  // Re-ensuring an existing joint returns the same id.
  EXPECT_EQ(analysis_.ensure_joint_id("b"), b);
}

// ===========================================================================
// Inverse producing-transmissions index
// ===========================================================================

TEST_F(TransmissionAnalysisTest, ProducingTransmissionsIndex_IsCorrect)
{
  analysis_.ensure_joint_id("a");
  analysis_.ensure_joint_id("b");
  const auto a_pos = analysis_.ensure_state_interface_id(
    NamedStateInterfaceDefinition{"a", InterfaceId{"position"}});
  const auto b_pos = analysis_.ensure_state_interface_id(
    NamedStateInterfaceDefinition{"b", InterfaceId{"position"}});

  const auto model_id = analysis_.add_model(std::make_unique<StubTransmissionModel>());
  analysis_.add_transmission(model_id, {a_pos}, {b_pos}, "T");

  const auto producers_b = analysis_.producing_transmissions(b_pos);
  ASSERT_EQ(producers_b.size(), 1u);
  EXPECT_EQ(producers_b[0], static_cast<TransmissionInstanceId>(0));

  // a_pos is an input, not an output, so its producer list should be empty.
  EXPECT_TRUE(analysis_.producing_transmissions(a_pos).empty());
}

// ===========================================================================
// Projection rule registry
// ===========================================================================

TEST_F(TransmissionAnalysisTest, AffineProjectionRule_DefaultsForPositionVelocityAcceleration)
{
  EXPECT_NE(analysis_.affine_projection_rule(InterfaceId{"position"}), nullptr);
  EXPECT_NE(analysis_.affine_projection_rule(InterfaceId{"velocity"}), nullptr);
  EXPECT_NE(analysis_.affine_projection_rule(InterfaceId{"acceleration"}), nullptr);

  // Effort is intentionally NOT registered by default.
  EXPECT_EQ(analysis_.affine_projection_rule(InterfaceId{"effort"}), nullptr);
}

TEST_F(TransmissionAnalysisTest, SetAffineProjectionRule_OverridesDefault)
{
  // Custom position rule with reverse_direction = true.
  analysis_.set_affine_projection_rule(
    InterfaceId{"position"},
    AffineProjectionRule{2.0, 3.0, true});

  const auto * rule = analysis_.affine_projection_rule(InterfaceId{"position"});
  ASSERT_NE(rule, nullptr);
  EXPECT_TRUE(approx_equal(rule->multiplier_scale, 2.0));
  EXPECT_TRUE(approx_equal(rule->offset_scale, 3.0));
  EXPECT_TRUE(rule->reverse_direction);
}
