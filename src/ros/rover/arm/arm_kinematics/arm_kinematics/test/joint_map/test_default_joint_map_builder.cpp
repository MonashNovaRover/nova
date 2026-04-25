//
// Created by Bailey Chessum on 9/4/26.
//
// End-to-end tests for DefaultJointMapBuilder. These exercise the canonical pipeline:
// populate the analysis, call build_expected, and either verify the JointMap produces correct
// values or pattern-match on the JointMapBuildError shape.
//

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "arm_kinematics/joint_map/compute_transmission.hpp"
#include "arm_kinematics/joint_map/default_joint_map_builder.hpp"
#include "arm_kinematics/joint_map/joint_map.hpp"
#include "arm_kinematics/joint_map/joint_map_builder.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_model.hpp"

using arm_kinematics::ComputeTransmission;
using arm_kinematics::DefaultJointMapBuilder;
using arm_kinematics::InterfaceId;
using arm_kinematics::JointId;
using arm_kinematics::JointMap;
using arm_kinematics::JointMapBuildError;
using arm_kinematics::NamedStateInterfaceDefinition;
using arm_kinematics::StateInterfaceDefinition;
using arm_kinematics::TransmissionAnalysis;
using arm_kinematics::TransmissionModel;
using StateInterfaceId = TransmissionAnalysis::StateInterfaceId;

namespace {

// Linear compute kernel: outputs = M * inputs + b. Same shape as the materializer test file.
class LinearCompute : public ComputeTransmission {
public:
  LinearCompute(std::vector<double> matrix, std::vector<double> bias,
                size_t input_count, size_t output_count)
    : matrix_(std::move(matrix)), bias_(std::move(bias)),
      input_count_(input_count), output_count_(output_count)
  {
  }
  void compute(
    arm_kinematics::span<const double> inputs,
    arm_kinematics::span<double> outputs,
    arm_kinematics::span<double>) const override
  {
    for (size_t i = 0; i < output_count_; ++i) {
      double acc = bias_[i];
      for (size_t j = 0; j < input_count_; ++j) {
        acc += matrix_[i * input_count_ + j] * inputs[j];
      }
      outputs[i] = acc;
    }
  }
  [[nodiscard]] size_t scratch_size() const noexcept override { return 0; }
  [[nodiscard]] std::unique_ptr<ComputeTransmission> clone() const override
  {
    return std::make_unique<LinearCompute>(matrix_, bias_, input_count_, output_count_);
  }

private:
  std::vector<double> matrix_;
  std::vector<double> bias_;
  size_t input_count_;
  size_t output_count_;
};

class LinearTransmissionModel : public TransmissionModel {
public:
  LinearTransmissionModel(std::vector<double> matrix, std::vector<double> bias,
                          size_t input_count, size_t output_count)
    : matrix_(std::move(matrix)), bias_(std::move(bias)),
      input_count_(input_count), output_count_(output_count)
  {
  }
  [[nodiscard]] std::unique_ptr<TransmissionModel> clone() const override
  {
    return std::make_unique<LinearTransmissionModel>(matrix_, bias_, input_count_, output_count_);
  }
  [[nodiscard]] std::unique_ptr<const ComputeTransmission> build(
    arm_kinematics::span<const StateInterfaceId>,
    arm_kinematics::span<const StateInterfaceId>) const override
  {
    return std::make_unique<LinearCompute>(matrix_, bias_, input_count_, output_count_);
  }

private:
  std::vector<double> matrix_;
  std::vector<double> bias_;
  size_t input_count_;
  size_t output_count_;
};

// Stub model used when the transmission's compute() body doesn't matter — only the topology
// is being asserted on (e.g., ambiguity-detection tests).
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
    class StubCompute : public ComputeTransmission {
    public:
      void compute(
        arm_kinematics::span<const double>,
        arm_kinematics::span<double>,
        arm_kinematics::span<double>) const override {}
      [[nodiscard]] size_t scratch_size() const noexcept override { return 0; }
      [[nodiscard]] std::unique_ptr<ComputeTransmission> clone() const override
      {
        return std::make_unique<StubCompute>();
      }
    };
    return std::make_unique<StubCompute>();
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

// Convert a StateInterfaceId back to a StateInterfaceDefinition via the analysis inverse index.
// Used in tests to build expected values for JointMapBuildError fields.
StateInterfaceDefinition def_of(const TransmissionAnalysis & analysis, const StateInterfaceId sid)
{
  return analysis.state_interface_order().inverse[sid];
}

// Named variant: resolves the joint name via joint_order for use with Named error fields.
NamedStateInterfaceDefinition named_def_of(
  const TransmissionAnalysis & analysis, const StateInterfaceId sid)
{
  const auto def = analysis.state_interface_order().inverse[sid];
  return {analysis.joint_order().inverse[def.joint_id], def.interface_id};
}

constexpr double kTolerance = 1.0e-9;

bool approx_equal(double a, double b)
{
  return std::abs(a - b) <= kTolerance + kTolerance * std::max(std::abs(a), std::abs(b));
}

const JointMapBuildError::MissingInputs & expect_missing_inputs(const JointMapBuildError & err)
{
  const auto * value = std::get_if<JointMapBuildError::MissingInputs>(&err.value);
  EXPECT_NE(value, nullptr);
  return *value;
}

const JointMapBuildError::Ambiguous & expect_ambiguous(const JointMapBuildError & err)
{
  const auto * value = std::get_if<JointMapBuildError::Ambiguous>(&err.value);
  EXPECT_NE(value, nullptr);
  return *value;
}

const JointMapBuildError::UnknownJoint & expect_unknown_joint(const JointMapBuildError & err)
{
  const auto * value = std::get_if<JointMapBuildError::UnknownJoint>(&err.value);
  EXPECT_NE(value, nullptr);
  return *value;
}

}  // namespace

class DefaultJointMapBuilderTest : public ::testing::Test
{
protected:
  // The builder holds a reference to the analysis. Construct the analysis first, then the
  // builder around it. Both have the same lifetime as the test instance.
  TransmissionAnalysis analysis_{};
  DefaultJointMapBuilder builder_{analysis_};
};

// ===========================================================================
// JointMap default-state test
// ===========================================================================

TEST(JointMapTests, DefaultConstructed_IsInvalid)
{
  arm_kinematics::JointMap joint_map{};

  EXPECT_FALSE(joint_map.valid());
  EXPECT_EQ(joint_map.input_count(), 0u);
  EXPECT_EQ(joint_map.output_count(), 0u);
}

// ===========================================================================
// Happy paths
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Success_PureAffineMimic)
{
  auto & analysis = analysis_;
  const auto joints = ensure_joints(analysis, {"j_a", "j_b"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis, "j_b", InterfaceId{"position"});
  analysis.add_affine_transmission(joints[0], joints[1], 2.0, 5.0);

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, a)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, a), def_of(analysis, b)});
  ASSERT_TRUE(result.has_value());

  const auto & jm = result.value();
  std::vector<double> in{3.0};
  std::vector<double> out(2, 0.0);
  jm.map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 3.0));
  EXPECT_TRUE(approx_equal(out[1], 11.0));  // 2*3 + 5
}

TEST_F(DefaultJointMapBuilderTest, Success_MixedAffineAndTransmission)
{
  auto & analysis = analysis_;
  const auto joints = ensure_joints(analysis, {"j_a", "j_b", "j_x", "j_y"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis, "j_b", InterfaceId{"position"});
  const auto x = ensure_state(analysis, "j_x", InterfaceId{"position"});
  const auto y = ensure_state(analysis, "j_y", InterfaceId{"position"});

  // b mimics a (b = 2a), y mimics x (y = 3x)
  analysis.add_affine_transmission(joints[0], joints[1], 2.0, 0.0);
  analysis.add_affine_transmission(joints[2], joints[3], 3.0, 0.0);
  // T: a → x, x = 5a + 1
  const auto m = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{5.0}, std::vector<double>{1.0}, 1, 1));
  analysis.add_transmission(m, {a}, {x}, "T");

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, a)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, b), def_of(analysis, x), def_of(analysis, y)});
  ASSERT_TRUE(result.has_value());

  std::vector<double> in{4.0};
  std::vector<double> out(3, 0.0);
  result.value().map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 8.0));   // b = 2*4
  EXPECT_TRUE(approx_equal(out[1], 21.0));  // x = 5*4 + 1
  EXPECT_TRUE(approx_equal(out[2], 63.0));  // y = 3*21
}

TEST_F(DefaultJointMapBuilderTest, Success_DefaultValueForBarePassivePosition)
{
  auto & analysis = analysis_;
  const auto runtime = analysis.ensure_joint_id("runtime");
  const auto passive = analysis.ensure_joint_id("passive");
  analysis.ensure_state_interface_id(StateInterfaceDefinition{runtime, InterfaceId::Position()});

  const DefaultJointMapBuilder builder{
    analysis,
    std::unordered_map<StateInterfaceDefinition, double>{
      {{passive, InterfaceId::Position()}, 2.5}}};

  const auto result = builder.build_expected(
    std::vector<StateInterfaceDefinition>{{runtime, InterfaceId::Position()}},
    std::vector<StateInterfaceDefinition>{{passive, InterfaceId::Position()}});
  ASSERT_TRUE(result.has_value());

  const auto & joint_map = result.value();
  EXPECT_EQ(joint_map.input_count(), 1u);
  EXPECT_EQ(joint_map.output_count(), 1u);

  std::vector<double> in{9.0};
  std::vector<double> out(1, 0.0);
  joint_map.map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 2.5));
}

TEST_F(DefaultJointMapBuilderTest, Success_BareMimicOutputsDeriveFromBareInputs)
{
  auto & analysis = analysis_;
  const auto j2 = analysis.ensure_joint_id("j2");
  const auto j3 = analysis.ensure_joint_id("j3");
  const auto l1_l2_pivot = analysis.ensure_joint_id("l1_l2_pivot");
  const auto l1_l2_pivot_mimic = analysis.ensure_joint_id("l1_l2_pivot_mimic");
  const auto l1_p_upper = analysis.ensure_joint_id("l1_p_upper");
  const auto l1_p_upper_mimic = analysis.ensure_joint_id("l1_p_upper_mimic");

  analysis.add_affine_transmission(j3, l1_l2_pivot, -1.0, 0.0);
  analysis.add_affine_transmission(j2, l1_l2_pivot_mimic, 1.0, 0.0);
  analysis.add_affine_transmission(j2, l1_p_upper, 1.0, 0.0);
  analysis.add_affine_transmission(j3, l1_p_upper_mimic, -1.0, 0.0);

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{
      {j2, InterfaceId::Position()},
      {j3, InterfaceId::Position()}},
    std::vector<StateInterfaceDefinition>{
      {l1_l2_pivot, InterfaceId::Position()},
      {l1_l2_pivot_mimic, InterfaceId::Position()},
      {l1_p_upper, InterfaceId::Position()},
      {l1_p_upper_mimic, InterfaceId::Position()}});
  ASSERT_TRUE(result.has_value());

  std::vector<double> in{1.25, 0.5};
  std::vector<double> out(4, 0.0);
  result.value().map(in, out);
  EXPECT_TRUE(approx_equal(out[0], -0.5));
  EXPECT_TRUE(approx_equal(out[1], 1.25));
  EXPECT_TRUE(approx_equal(out[2], 1.25));
  EXPECT_TRUE(approx_equal(out[3], -0.5));
}

// ===========================================================================
// MissingInputs error
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Error_MissingInputs_OutputUnreachable)
{
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_a", "j_b", "j_c"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis, "j_c", InterfaceId{"position"});

  const auto m = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{1.0, 1.0}, std::vector<double>{0.0}, 2, 1));
  analysis.add_transmission(m, {a, b}, {c}, "T");

  // Only `a` supplied; T can't fire so c is unproducible.
  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, a)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, c)});

  ASSERT_FALSE(result.has_value());
  const auto & err = expect_missing_inputs(result.error());
  ASSERT_EQ(err.unproducible_outputs.size(), 1u);
  EXPECT_EQ(err.unproducible_outputs[0], named_def_of(analysis, c));
  EXPECT_EQ(err.resolutions.size(), 1u);  // stub returns one entry per missing
  EXPECT_EQ(err.resolutions[0].missing, named_def_of(analysis, c));
}

// ===========================================================================
// Ambiguous error: directly ambiguous output
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Error_DirectAmbiguity_OutputItselfAmbiguous)
{
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_a", "j_x"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis, "j_x", InterfaceId{"position"});

  const auto m = analysis.add_model(std::make_unique<StubTransmissionModel>());
  analysis.add_transmission(m, {a}, {x}, "T1");
  analysis.add_transmission(m, {a}, {x}, "T2");

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, a)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, x)});

  ASSERT_FALSE(result.has_value());
  const auto & err = expect_ambiguous(result.error());
  ASSERT_EQ(err.ambiguous_interfaces.size(), 1u);
  EXPECT_EQ(err.ambiguous_interfaces[0].interface, def_of(analysis, x));
  EXPECT_EQ(err.ambiguous_interfaces[0].candidates.size(), 2u);
}

// ===========================================================================
// Ambiguous error: transitive ambiguity poison
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Error_TransitiveAmbiguity_OutputDownstreamOfAmbiguous)
{
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_a", "j_x", "j_y"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis, "j_x", InterfaceId{"position"});
  const auto y = ensure_state(analysis, "j_y", InterfaceId{"position"});

  const auto m = analysis.add_model(std::make_unique<StubTransmissionModel>());
  analysis.add_transmission(m, {a}, {x}, "T1");
  analysis.add_transmission(m, {a}, {x}, "T2");  // x is ambiguous
  analysis.add_transmission(m, {x}, {y}, "T3");  // y depends on x

  // Asking for y should fail with Ambiguous because y transitively depends on x.
  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, a)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, y)});

  ASSERT_FALSE(result.has_value());
  const auto & err = expect_ambiguous(result.error());
  ASSERT_EQ(err.ambiguous_interfaces.size(), 1u);
  EXPECT_EQ(err.ambiguous_interfaces[0].interface, def_of(analysis, x));  // The relevant ambiguity
}

// ===========================================================================
// Unrelated ambiguity should NOT prevent the build
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Success_UnrelatedAmbiguity_DoesNotBlockUnrelatedRequest)
{
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_a", "j_x", "j_z"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis, "j_x", InterfaceId{"position"});
  const auto z = ensure_state(analysis, "j_z", InterfaceId{"position"});

  // T1, T2 both produce x → x is ambiguous (but NOT requested)
  const auto stub = analysis.add_model(std::make_unique<StubTransmissionModel>());
  analysis.add_transmission(stub, {a}, {x}, "T1");
  analysis.add_transmission(stub, {a}, {x}, "T2");
  // T3 produces z uniquely (using a real linear kernel so we can verify the value)
  const auto linear = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{7.0}, std::vector<double>{2.0}, 1, 1));
  analysis.add_transmission(linear, {a}, {z}, "T3");

  // Asking only for z should succeed despite x being ambiguous in the reachability.
  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, a)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, z)});

  ASSERT_TRUE(result.has_value());
  std::vector<double> in{4.0};
  std::vector<double> out(1, 0.0);
  result.value().map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 30.0));  // 7*4 + 2
}

// ===========================================================================
// Polymorphic call through the JointMapBuilder base
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, PolymorphicCallThroughBaseInterface)
{
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_a"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});

  const arm_kinematics::JointMapBuilder & base = builder_;
  const auto result = base.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis_, a)},
    std::vector<StateInterfaceDefinition>{def_of(analysis_, a)});

  ASSERT_TRUE(result.has_value());
  std::vector<double> in{42.0};
  std::vector<double> out(1, 0.0);
  result.value().map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 42.0));
}

// ===========================================================================
// Unknown interface
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Error_UnknownJoint_BogusOutputJointId)
{
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_a"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});

  // Pass a definition referencing an out-of-range JointId as an output.
  const JointId bogus_joint = 999;
  const StateInterfaceDefinition bogus_def{bogus_joint, InterfaceId{"position"}};
  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, a)},
    std::vector<StateInterfaceDefinition>{bogus_def});

  ASSERT_FALSE(result.has_value());
  const auto & err = expect_unknown_joint(result.error());
  ASSERT_EQ(err.unknown_joints.size(), 1u);
  EXPECT_EQ(err.unknown_joints[0], "<unknown id=999>");
}

TEST_F(DefaultJointMapBuilderTest, Error_UnknownJoint_BogusInputJointId)
{
  // Same but the bogus joint id is in the inputs list — should also be caught.
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_a"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});

  const JointId bogus_joint = 42;
  const StateInterfaceDefinition bogus_def{bogus_joint, InterfaceId{"position"}};
  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{bogus_def},
    std::vector<StateInterfaceDefinition>{def_of(analysis, a)});

  ASSERT_FALSE(result.has_value());
  const auto & err = expect_unknown_joint(result.error());
  EXPECT_EQ(err.unknown_joints[0], "<unknown id=42>");
}

// ===========================================================================
// Double build (no cached state contamination)
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, DoubleBuild_NoCachedStateContamination)
{
  auto & analysis = analysis_;
  const auto joints = ensure_joints(analysis, {"j_a", "j_b"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis, "j_b", InterfaceId{"position"});
  analysis.add_affine_transmission(joints[0], joints[1], 2.0, 0.0);

  // First call: request just `a`.
  const auto result1 = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, a)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, a)});
  ASSERT_TRUE(result1.has_value());

  // Second call on the same builder: different request (now ask for `b`).
  const auto result2 = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, a)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, b)});
  ASSERT_TRUE(result2.has_value());

  std::vector<double> in{5.0};
  std::vector<double> out1(1, 0.0);
  result1.value().map(in, out1);
  EXPECT_TRUE(approx_equal(out1[0], 5.0));

  std::vector<double> out2(1, 0.0);
  result2.value().map(in, out2);
  EXPECT_TRUE(approx_equal(out2[0], 10.0));  // 2*5
}

// ===========================================================================
// Velocity projection rule end-to-end
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Success_VelocityProjectionRule_EndToEnd)
{
  // b mimics a (b = 2a + 5). Request b.velocity from a.velocity → expect 2*v_a (offset
  // dropped by the velocity rule).
  auto & analysis = analysis_;
  const auto joints = ensure_joints(analysis, {"j_a", "j_b"});
  const auto a_vel = ensure_state(analysis, "j_a", InterfaceId{"velocity"});
  const auto b_vel = ensure_state(analysis, "j_b", InterfaceId{"velocity"});
  analysis.add_affine_transmission(joints[0], joints[1], 2.0, 5.0);

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, a_vel)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, b_vel)});
  ASSERT_TRUE(result.has_value());

  std::vector<double> in{3.0};
  std::vector<double> out(1, 0.0);
  result.value().map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 6.0));  // 2*3, offset dropped
}

// ===========================================================================
// Diamond DAG end-to-end numerical verification
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Success_DiamondDAG_NumericalVerification)
{
  // T1: a → b = 2a
  // T2: a → c = 3a + 1
  // T3: b, c → d = b + c
  // For a = 4: b = 8, c = 13, d = 21.
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_a", "j_b", "j_c", "j_d"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis, "j_c", InterfaceId{"position"});
  const auto d = ensure_state(analysis, "j_d", InterfaceId{"position"});

  const auto m1 = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{2.0}, std::vector<double>{0.0}, 1, 1));
  const auto m2 = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{3.0}, std::vector<double>{1.0}, 1, 1));
  const auto m3 = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{1.0, 1.0}, std::vector<double>{0.0}, 2, 1));
  analysis.add_transmission(m1, {a}, {b}, "T1");
  analysis.add_transmission(m2, {a}, {c}, "T2");
  analysis.add_transmission(m3, {b, c}, {d}, "T3");

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, a)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, d)});
  ASSERT_TRUE(result.has_value());

  std::vector<double> in{4.0};
  std::vector<double> out(1, 0.0);
  result.value().map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 21.0));
}

// ===========================================================================
// Redundant equivalent inputs end-to-end
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Success_RedundantEquivalentInputs_EndToEnd)
{
  // A→B (B=2A) and A→D (D=3A). Supply both A and D. Request B.
  // Expected: B = 2A computed correctly. The redundant supply of D doesn't break the build.
  auto & analysis = analysis_;
  const auto joints = ensure_joints(analysis, {"j_a", "j_b", "j_d"});
  const auto a_pos = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto b_pos = ensure_state(analysis, "j_b", InterfaceId{"position"});
  const auto d_pos = ensure_state(analysis, "j_d", InterfaceId{"position"});
  analysis.add_affine_transmission(joints[0], joints[1], 2.0, 0.0);
  analysis.add_affine_transmission(joints[0], joints[2], 3.0, 0.0);

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, a_pos), def_of(analysis, d_pos)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, b_pos)});
  ASSERT_TRUE(result.has_value());

  std::vector<double> in{5.0, 15.0};  // a=5, d=15 (consistent: 3*5=15)
  std::vector<double> out(1, 0.0);
  result.value().map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 10.0));  // B = 2*A = 10
}

// ===========================================================================
// Transmission with mixed gather sources (input slot + upstream transmission output)
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Success_TransmissionWithMixedGatherSources)
{
  // T1: a → b = 2a
  // T2: b, c → d = b + c   (where c is also a direct input)
  // Inputs: {a, c}. Request: {d}.
  // For a=3, c=10: b=6, d=16.
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_a", "j_b", "j_c", "j_d"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis, "j_c", InterfaceId{"position"});
  const auto d = ensure_state(analysis, "j_d", InterfaceId{"position"});

  const auto m1 = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{2.0}, std::vector<double>{0.0}, 1, 1));
  const auto m2 = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{1.0, 1.0}, std::vector<double>{0.0}, 2, 1));
  analysis.add_transmission(m1, {a}, {b}, "T1");
  analysis.add_transmission(m2, {b, c}, {d}, "T2");

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, a), def_of(analysis, c)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, d)});
  ASSERT_TRUE(result.has_value());

  std::vector<double> in{3.0, 10.0};
  std::vector<double> out(1, 0.0);
  result.value().map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 16.0));
}

// ===========================================================================
// TransmissionModel returning null compute
// ===========================================================================

namespace {

class NullReturningModel : public TransmissionModel {
public:
  [[nodiscard]] std::unique_ptr<TransmissionModel> clone() const override
  {
    return std::make_unique<NullReturningModel>();
  }
  [[nodiscard]] std::unique_ptr<const ComputeTransmission> build(
    arm_kinematics::span<const StateInterfaceId>,
    arm_kinematics::span<const StateInterfaceId>) const override
  {
    return nullptr;
  }
};

}  // namespace

TEST_F(DefaultJointMapBuilderTest, Error_TransmissionModelReturnsNull_PropagatesAsException)
{
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_a", "j_b"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis, "j_b", InterfaceId{"position"});

  const auto m = analysis.add_model(std::make_unique<NullReturningModel>());
  analysis.add_transmission(m, {a}, {b}, "T");

  // The materializer should throw std::invalid_argument when build() returns null.
  EXPECT_THROW(
    {
      auto result = builder_.build_expected(
        std::vector<StateInterfaceDefinition>{def_of(analysis, a)},
        std::vector<StateInterfaceDefinition>{def_of(analysis, b)});
      (void)result;
    },
    std::invalid_argument);
}

// ===========================================================================
// Second-order ambiguity reported through build_expected
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Error_SecondOrderAmbiguity_ReportedThroughBuildExpected)
{
  // Same scenario as Ambiguity_DownstreamProducerWithCleanAlternativeStaysAmbiguous in the
  // reachability tests. Verify the builder reports x as ambiguous (with both T1 and T2 in
  // the snapshot) AND surfaces b as a relevant blocking ambiguity.
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_a", "j_b", "j_c", "j_x"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis, "j_c", InterfaceId{"position"});
  const auto x = ensure_state(analysis, "j_x", InterfaceId{"position"});

  const auto m = analysis.add_model(std::make_unique<StubTransmissionModel>());
  analysis.add_transmission(m, {a}, {x}, "T1");
  analysis.add_transmission(m, {b}, {x}, "T2");
  analysis.add_transmission(m, {c}, {b}, "T3");
  analysis.add_transmission(m, {c}, {b}, "T4");

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, a), def_of(analysis, c)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, x)});

  ASSERT_FALSE(result.has_value());
  const auto & err = expect_ambiguous(result.error());
  // x and b should both appear in ambiguous_interfaces (x directly, b transitively).
  // Note: the exact contents depend on the diagnose attribution walk; both should be
  // reported because x is directly requested and b is in x's potential producer chain
  // through T2.
  bool saw_x = false, saw_b = false;
  for (const auto & ai : err.ambiguous_interfaces) {
    if (ai.interface == def_of(analysis, x)) {
      saw_x = true;
      EXPECT_EQ(ai.candidates.size(), 2u);  // T1 and T2
    }
    if (ai.interface == def_of(analysis, b)) saw_b = true;
  }
  EXPECT_TRUE(saw_x);
  EXPECT_TRUE(saw_b);
}

// ===========================================================================
// Forward/inverse transmission pairs (the analysis is not a DAG and that's OK)
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Success_ForwardInversePair_MotorToJoint)
{
  // T_fwd: motor → joint, joint = 5*motor.
  // T_inv: joint → motor, motor = joint/5 (i.e., 0.2*joint).
  // Inputs: {motor}, request: {joint}.
  // Expected: joint = 5 * motor.
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_motor", "j_joint"});
  const auto motor = ensure_state(analysis, "j_motor", InterfaceId{"position"});
  const auto joint = ensure_state(analysis, "j_joint", InterfaceId{"position"});

  const auto m_fwd = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{5.0}, std::vector<double>{0.0}, 1, 1));
  const auto m_inv = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{0.2}, std::vector<double>{0.0}, 1, 1));
  analysis.add_transmission(m_fwd, {motor}, {joint}, "T_fwd");
  analysis.add_transmission(m_inv, {joint}, {motor}, "T_inv");

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, motor)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, joint)});
  ASSERT_TRUE(result.has_value());

  std::vector<double> in{3.0};
  std::vector<double> out(1, 0.0);
  result.value().map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 15.0));  // joint = 5*3
}

TEST_F(DefaultJointMapBuilderTest, Success_ForwardInversePair_JointToMotor)
{
  // Same setup, opposite direction. Inputs: {joint}, request: {motor}.
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_motor", "j_joint"});
  const auto motor = ensure_state(analysis, "j_motor", InterfaceId{"position"});
  const auto joint = ensure_state(analysis, "j_joint", InterfaceId{"position"});

  const auto m_fwd = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{5.0}, std::vector<double>{0.0}, 1, 1));
  const auto m_inv = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{0.2}, std::vector<double>{0.0}, 1, 1));
  analysis.add_transmission(m_fwd, {motor}, {joint}, "T_fwd");
  analysis.add_transmission(m_inv, {joint}, {motor}, "T_inv");

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, joint)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, motor)});
  ASSERT_TRUE(result.has_value());

  std::vector<double> in{20.0};
  std::vector<double> out(1, 0.0);
  result.value().map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 4.0));  // motor = 0.2*20
}

TEST_F(DefaultJointMapBuilderTest, Success_ForwardInversePair_BothInputs_NoAmbiguity)
{
  // Both motor and joint supplied as inputs. The user asks for both back. Both should be
  // input passthroughs, no ambiguity reported (input wins on both sides).
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_motor", "j_joint"});
  const auto motor = ensure_state(analysis, "j_motor", InterfaceId{"position"});
  const auto joint = ensure_state(analysis, "j_joint", InterfaceId{"position"});

  const auto m_fwd = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{5.0}, std::vector<double>{0.0}, 1, 1));
  const auto m_inv = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{0.2}, std::vector<double>{0.0}, 1, 1));
  analysis.add_transmission(m_fwd, {motor}, {joint}, "T_fwd");
  analysis.add_transmission(m_inv, {joint}, {motor}, "T_inv");

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, motor), def_of(analysis, joint)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, motor), def_of(analysis, joint)});
  ASSERT_TRUE(result.has_value());

  // The user supplies inconsistent values (motor=3, joint=99) intentionally — input wins on
  // both sides means the joint map passes them through unchanged.
  std::vector<double> in{3.0, 99.0};
  std::vector<double> out(2, 0.0);
  result.value().map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 3.0));   // motor passthrough
  EXPECT_TRUE(approx_equal(out[1], 99.0));  // joint passthrough
}

TEST_F(DefaultJointMapBuilderTest, Success_InverseTransmissionWithSideEffectOutput)
{
  // T_fwd: motor → joint = 5*motor.
  // T_inv: joint → {motor, motor_torque}.
  //   motor_torque = 0.1 * joint   (some side-effect output of the inverse transmission)
  // Inputs: {motor}, request: {joint, motor_torque}.
  //
  // The forward direction fires T_fwd to derive joint. The inverse direction (T_inv) is
  // needed to derive motor_torque (since nothing else produces it, and joint becomes
  // derivable after T_fwd). Both transmissions are selected; both fire at runtime.
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_motor", "j_joint", "j_motor_torque"});
  const auto motor = ensure_state(analysis, "j_motor", InterfaceId{"position"});
  const auto joint = ensure_state(analysis, "j_joint", InterfaceId{"position"});
  const auto motor_torque = ensure_state(analysis, "j_motor_torque", InterfaceId{"position"});

  // T_fwd: 1 input → 1 output (joint = 5*motor)
  const auto m_fwd = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{5.0}, std::vector<double>{0.0}, 1, 1));
  // T_inv: 1 input → 2 outputs ([motor, motor_torque] = [0.2*joint, 0.1*joint])
  const auto m_inv = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{0.2, 0.1}, std::vector<double>{0.0, 0.0}, 1, 2));
  analysis.add_transmission(m_fwd, {motor}, {joint}, "T_fwd");
  analysis.add_transmission(m_inv, {joint}, {motor, motor_torque}, "T_inv");

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, motor)},
    std::vector<StateInterfaceDefinition>{def_of(analysis, joint), def_of(analysis, motor_torque)});
  ASSERT_TRUE(result.has_value());

  std::vector<double> in{4.0};  // motor=4
  std::vector<double> out(2, 0.0);
  result.value().map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 20.0));  // joint = 5*4
  EXPECT_TRUE(approx_equal(out[1], 2.0));   // motor_torque = 0.1*joint = 0.1*20
}

// ===========================================================================
// Empty inputs + non-empty outputs
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Error_EmptyInputs_AllOutputsUnreachable)
{
  // No inputs supplied; ask for an output that has no producer.
  auto & analysis = analysis_;
  ensure_joints(analysis, {"j_a"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{},
    std::vector<StateInterfaceDefinition>{def_of(analysis, a)});

  ASSERT_FALSE(result.has_value());
  const auto & err = expect_missing_inputs(result.error());
  ASSERT_EQ(err.unproducible_outputs.size(), 1u);
  EXPECT_EQ(err.unproducible_outputs[0], named_def_of(analysis, a));
}

// ===========================================================================
// Multi-ambiguous outputs through build_expected (exercises the message truncation)
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Error_ManyAmbiguousOutputs_MessageIsTruncatedSensibly)
{
  // Set up 8 ambiguous outputs (more than the 5-entry message cap).
  auto & analysis = analysis_;
  std::vector<StateInterfaceId> output_sids;
  for (int i = 0; i < 8; ++i) {
    const std::string joint_name = "j_x" + std::to_string(i);
    ensure_joints(analysis, {joint_name.c_str()});
    output_sids.push_back(ensure_state(analysis, joint_name, InterfaceId{"position"}));
  }
  ensure_joints(analysis, {"j_a"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});

  const auto m = analysis.add_model(std::make_unique<StubTransmissionModel>());
  for (int i = 0; i < 8; ++i) {
    analysis.add_transmission(m, {a}, {output_sids[i]}, "T1_" + std::to_string(i));
    analysis.add_transmission(m, {a}, {output_sids[i]}, "T2_" + std::to_string(i));
  }

  std::vector<StateInterfaceDefinition> outputs;
  outputs.reserve(output_sids.size());
  for (const auto sid : output_sids) {
    outputs.push_back(def_of(analysis, sid));
  }

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceDefinition>{def_of(analysis, a)}, outputs);

  ASSERT_FALSE(result.has_value());
  const auto & err = expect_ambiguous(result.error());
  // All 8 should be in ambiguous_interfaces.
  EXPECT_EQ(err.ambiguous_interfaces.size(), 8u);
  // Message should reference truncation ("...and N more") when listing 8 entries (cap is 5).
  EXPECT_NE(result.error().format().find("and 3 more"), std::string::npos);
  EXPECT_NE(result.error().format().find("8 ambiguous interface"), std::string::npos);
}
