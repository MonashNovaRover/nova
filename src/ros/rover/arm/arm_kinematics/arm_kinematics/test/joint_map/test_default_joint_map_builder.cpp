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
#include <string>
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
using arm_kinematics::StateInterfaceId;
using arm_kinematics::TransmissionAnalysis;
using arm_kinematics::TransmissionModel;

namespace {

// Linear compute kernel: outputs = M * inputs + b. Same shape as the materializer test file.
class LinearCompute : public ComputeTransmission {
public:
  LinearCompute(std::vector<float> matrix, std::vector<float> bias,
                size_t input_count, size_t output_count)
    : matrix_(std::move(matrix)), bias_(std::move(bias)),
      input_count_(input_count), output_count_(output_count)
  {
  }
  void compute(
    arm_kinematics::span<const float> inputs,
    arm_kinematics::span<float> outputs,
    arm_kinematics::span<float>) const override
  {
    for (size_t i = 0; i < output_count_; ++i) {
      float acc = bias_[i];
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
  std::vector<float> matrix_;
  std::vector<float> bias_;
  size_t input_count_;
  size_t output_count_;
};

class LinearTransmissionModel : public TransmissionModel {
public:
  LinearTransmissionModel(std::vector<float> matrix, std::vector<float> bias,
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
  std::vector<float> matrix_;
  std::vector<float> bias_;
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
        arm_kinematics::span<const float>,
        arm_kinematics::span<float>,
        arm_kinematics::span<float>) const override {}
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

constexpr float kTolerance = 1.0e-5F;

bool approx_equal(float a, float b)
{
  return std::abs(a - b) <= kTolerance + kTolerance * std::max(std::abs(a), std::abs(b));
}

}  // namespace

class DefaultJointMapBuilderTest : public ::testing::Test
{
protected:
  DefaultJointMapBuilder builder_{};
};

// ===========================================================================
// Happy paths
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Success_PureAffineMimic)
{
  auto & analysis = builder_.get_mutable_transmission_analysis();
  const auto joints = ensure_joints(analysis, {"j_a", "j_b"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis, "j_b", InterfaceId{"position"});
  analysis.add_affine_transmission(joints[0], joints[1], 2.0F, 5.0F);

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceId>{a},
    std::vector<StateInterfaceId>{a, b});
  ASSERT_TRUE(result.has_value());

  const auto & jm = result.value();
  std::vector<float> in{3.0F};
  std::vector<float> out(2, 0.0F);
  jm.map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 3.0F));
  EXPECT_TRUE(approx_equal(out[1], 11.0F));  // 2*3 + 5
}

TEST_F(DefaultJointMapBuilderTest, Success_MixedAffineAndTransmission)
{
  auto & analysis = builder_.get_mutable_transmission_analysis();
  const auto joints = ensure_joints(analysis, {"j_a", "j_b", "j_x", "j_y"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis, "j_b", InterfaceId{"position"});
  const auto x = ensure_state(analysis, "j_x", InterfaceId{"position"});
  const auto y = ensure_state(analysis, "j_y", InterfaceId{"position"});

  // b mimics a (b = 2a), y mimics x (y = 3x)
  analysis.add_affine_transmission(joints[0], joints[1], 2.0F, 0.0F);
  analysis.add_affine_transmission(joints[2], joints[3], 3.0F, 0.0F);
  // T: a → x, x = 5a + 1
  const auto m = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<float>{5.0F}, std::vector<float>{1.0F}, 1, 1));
  analysis.add_transmission(m, {a}, {x}, "T");

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceId>{a},
    std::vector<StateInterfaceId>{b, x, y});
  ASSERT_TRUE(result.has_value());

  std::vector<float> in{4.0F};
  std::vector<float> out(3, 0.0F);
  result.value().map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 8.0F));   // b = 2*4
  EXPECT_TRUE(approx_equal(out[1], 21.0F));  // x = 5*4 + 1
  EXPECT_TRUE(approx_equal(out[2], 63.0F));  // y = 3*21
}

// ===========================================================================
// MissingInputs error
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Error_MissingInputs_OutputUnreachable)
{
  auto & analysis = builder_.get_mutable_transmission_analysis();
  ensure_joints(analysis, {"j_a", "j_b", "j_c"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis, "j_c", InterfaceId{"position"});

  const auto m = analysis.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<float>{1.0F, 1.0F}, std::vector<float>{0.0F}, 2, 1));
  analysis.add_transmission(m, {a, b}, {c}, "T");

  // Only `a` supplied; T can't fire so c is unreachable.
  const auto result = builder_.build_expected(
    std::vector<StateInterfaceId>{a},
    std::vector<StateInterfaceId>{c});

  ASSERT_FALSE(result.has_value());
  const auto & err = result.error();
  EXPECT_EQ(err.kind, JointMapBuildError::Kind::MissingInputs);
  ASSERT_EQ(err.unreachable_outputs.size(), 1u);
  EXPECT_EQ(err.unreachable_outputs[0], c);
  EXPECT_EQ(err.resolutions.size(), 1u);  // stub returns one entry per missing
  EXPECT_EQ(err.resolutions[0].missing, c);
}

// ===========================================================================
// Ambiguous error: directly ambiguous output
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Error_DirectAmbiguity_OutputItselfAmbiguous)
{
  auto & analysis = builder_.get_mutable_transmission_analysis();
  ensure_joints(analysis, {"j_a", "j_x"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis, "j_x", InterfaceId{"position"});

  const auto m = analysis.add_model(std::make_unique<StubTransmissionModel>());
  analysis.add_transmission(m, {a}, {x}, "T1");
  analysis.add_transmission(m, {a}, {x}, "T2");

  const auto result = builder_.build_expected(
    std::vector<StateInterfaceId>{a},
    std::vector<StateInterfaceId>{x});

  ASSERT_FALSE(result.has_value());
  const auto & err = result.error();
  EXPECT_EQ(err.kind, JointMapBuildError::Kind::Ambiguous);
  ASSERT_EQ(err.ambiguous_interfaces.size(), 1u);
  EXPECT_EQ(err.ambiguous_interfaces[0].interface, x);
  EXPECT_EQ(err.ambiguous_interfaces[0].candidates.size(), 2u);
}

// ===========================================================================
// Ambiguous error: transitive ambiguity poison
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Error_TransitiveAmbiguity_OutputDownstreamOfAmbiguous)
{
  auto & analysis = builder_.get_mutable_transmission_analysis();
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
    std::vector<StateInterfaceId>{a},
    std::vector<StateInterfaceId>{y});

  ASSERT_FALSE(result.has_value());
  const auto & err = result.error();
  EXPECT_EQ(err.kind, JointMapBuildError::Kind::Ambiguous);
  ASSERT_EQ(err.ambiguous_interfaces.size(), 1u);
  EXPECT_EQ(err.ambiguous_interfaces[0].interface, x);  // The relevant ambiguity
}

// ===========================================================================
// Unrelated ambiguity should NOT prevent the build
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, Success_UnrelatedAmbiguity_DoesNotBlockUnrelatedRequest)
{
  auto & analysis = builder_.get_mutable_transmission_analysis();
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
    std::vector<float>{7.0F}, std::vector<float>{2.0F}, 1, 1));
  analysis.add_transmission(linear, {a}, {z}, "T3");

  // Asking only for z should succeed despite x being ambiguous in the reachability.
  const auto result = builder_.build_expected(
    std::vector<StateInterfaceId>{a},
    std::vector<StateInterfaceId>{z});

  ASSERT_TRUE(result.has_value());
  std::vector<float> in{4.0F};
  std::vector<float> out(1, 0.0F);
  result.value().map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 30.0F));  // 7*4 + 2
}

// ===========================================================================
// Polymorphic call through the JointMapBuilder base
// ===========================================================================

TEST_F(DefaultJointMapBuilderTest, PolymorphicCallThroughBaseInterface)
{
  auto & analysis = builder_.get_mutable_transmission_analysis();
  ensure_joints(analysis, {"j_a"});
  const auto a = ensure_state(analysis, "j_a", InterfaceId{"position"});

  const arm_kinematics::JointMapBuilder & base = builder_;
  const auto result = base.build_expected(
    std::vector<StateInterfaceId>{a},
    std::vector<StateInterfaceId>{a});

  ASSERT_TRUE(result.has_value());
  std::vector<float> in{42.0F};
  std::vector<float> out(1, 0.0F);
  result.value().map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 42.0F));
}
