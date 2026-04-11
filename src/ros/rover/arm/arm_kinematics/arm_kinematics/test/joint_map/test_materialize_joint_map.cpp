//
// Created by Bailey Chessum on 8/4/26.
//
// End-to-end tests for materialize_joint_map. These exercise the full pipeline:
// TransmissionReachability → JointMapBlueprint → materialize → runtime JointMap.map().
// Each test sets up a small TransmissionAnalysis with real (numerically meaningful)
// ComputeTransmission implementations and asserts that the runtime joint map produces
// the expected outputs.
//

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <vector>

#include "arm_kinematics/joint_map/affine_joint_map.hpp"
#include "arm_kinematics/joint_map/composite_joint_map.hpp"
#include "arm_kinematics/joint_map/compute_transmission.hpp"
#include "arm_kinematics/joint_map/joint_map_blueprint.hpp"
#include "arm_kinematics/joint_map/materialize_joint_map.hpp"
#include "arm_kinematics/joint_map/missing_input_resolution.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_joint_map.hpp"
#include "arm_kinematics/joint_map/transmission_model.hpp"
#include "arm_kinematics/joint_map/transmission_reachability.hpp"

using arm_kinematics::AffineJointMap;
using arm_kinematics::ComputeTransmission;
using arm_kinematics::InterfaceId;
using arm_kinematics::JointId;
using arm_kinematics::JointMap;
using arm_kinematics::NamedStateInterfaceDefinition;
using arm_kinematics::StateInterfaceId;
using arm_kinematics::TransmissionAnalysis;
using arm_kinematics::TransmissionJointMap;
using arm_kinematics::TransmissionModel;
using arm_kinematics::TransmissionReachability;

namespace {

// ---------------------------------------------------------------------------
// Test compute kernels
// ---------------------------------------------------------------------------

// Computes a linear transformation: outputs = M * inputs + b, where M is row-major.
class LinearCompute : public ComputeTransmission {
public:
  LinearCompute(std::vector<double> matrix, std::vector<double> bias, size_t input_count, size_t output_count)
    : matrix_(std::move(matrix)),
      bias_(std::move(bias)),
      input_count_(input_count),
      output_count_(output_count)
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

// Wraps a captured LinearCompute so the analysis can hand it back at build time.
class LinearTransmissionModel : public TransmissionModel {
public:
  explicit LinearTransmissionModel(std::vector<double> matrix, std::vector<double> bias,
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

constexpr double kTolerance = 1.0e-9;

bool approx_equal(double a, double b)
{
  return std::abs(a - b) <= kTolerance + kTolerance * std::max(std::abs(a), std::abs(b));
}

// One-shot helper: build the reachability + plan + materialize, then run map().
JointMap build_joint_map(
  const TransmissionAnalysis & analysis,
  const std::vector<StateInterfaceId> & inputs,
  const std::vector<StateInterfaceId> & outputs)
{
  const auto reach = TransmissionReachability::analyze(analysis, inputs);
  const auto blueprint = arm_kinematics::plan_joint_map(reach, outputs);
  return arm_kinematics::materialize_joint_map(blueprint, analysis);
}

}  // namespace

class MaterializeJointMapTest : public ::testing::Test
{
protected:
  TransmissionAnalysis analysis_{};
};

// ===========================================================================
// Pure affine: identity passthrough
// ===========================================================================

TEST_F(MaterializeJointMapTest, PureAffine_IdentityPassthrough)
{
  ensure_joints(analysis_, {"j_a"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});

  const auto jm = build_joint_map(analysis_, {a}, {a});

  EXPECT_EQ(jm.input_count(), 1u);
  EXPECT_EQ(jm.output_count(), 1u);

  std::vector<double> in{5.0};
  std::vector<double> out(1, 0.0);
  jm.map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 5.0));
}

// ===========================================================================
// Pure affine: single mimic (b = 2a + 3)
// ===========================================================================

TEST_F(MaterializeJointMapTest, PureAffine_SingleMimic)
{
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0, 3.0);

  const auto jm = build_joint_map(analysis_, {a}, {b});

  std::vector<double> in{5.0};
  std::vector<double> out(1, 0.0);
  jm.map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 13.0));  // 2*5 + 3 = 13
}

// ===========================================================================
// Pure affine: multiple mimics from one input — verifies the affine batching
// fast path produces a single AffineJointMap with N rows
// ===========================================================================

TEST_F(MaterializeJointMapTest, PureAffine_MultipleMimics_OneInputManyOutputs)
{
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b1", "j_b2", "j_b3"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b1 = ensure_state(analysis_, "j_b1", InterfaceId{"position"});
  const auto b2 = ensure_state(analysis_, "j_b2", InterfaceId{"position"});
  const auto b3 = ensure_state(analysis_, "j_b3", InterfaceId{"position"});

  analysis_.add_affine_transmission(joints[0], joints[1], 2.0, 0.0);
  analysis_.add_affine_transmission(joints[0], joints[2], 3.0, 1.0);
  analysis_.add_affine_transmission(joints[0], joints[3], 0.5, -2.0);

  const auto jm = build_joint_map(analysis_, {a}, {b1, b2, b3});

  std::vector<double> in{4.0};
  std::vector<double> out(3, 0.0);
  jm.map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 8.0));   // 2 * 4
  EXPECT_TRUE(approx_equal(out[1], 13.0));  // 3 * 4 + 1
  EXPECT_TRUE(approx_equal(out[2], 0.0));   // 0.5 * 4 - 2
}

// ===========================================================================
// Pure affine: passthrough mixed with mimic in the same output request
// ===========================================================================

TEST_F(MaterializeJointMapTest, PureAffine_PassthroughAndMimic)
{
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  analysis_.add_affine_transmission(joints[0], joints[1], 2.0, 5.0);

  const auto jm = build_joint_map(analysis_, {a}, {a, b});

  std::vector<double> in{3.0};
  std::vector<double> out(2, 0.0);
  jm.map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 3.0));   // a passthrough
  EXPECT_TRUE(approx_equal(out[1], 11.0));  // 2*3 + 5
}

// ===========================================================================
// Mixed: single transmission (1 input → 1 output, x = 5a)
// ===========================================================================

TEST_F(MaterializeJointMapTest, Mixed_SingleTransmission_OneInputOneOutput)
{
  ensure_joints(analysis_, {"j_a", "j_x"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});

  // x = 5*a
  const auto model_id = analysis_.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{5.0}, std::vector<double>{0.0}, 1, 1));
  analysis_.add_transmission(model_id, {a}, {x}, "T");

  const auto jm = build_joint_map(analysis_, {a}, {x});

  std::vector<double> in{2.0};
  std::vector<double> out(1, 0.0);
  jm.map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 10.0));
}

// ===========================================================================
// Mixed: transmission followed by an affine projection of its output
// ===========================================================================

TEST_F(MaterializeJointMapTest, Mixed_TransmissionThenAffineProjection)
{
  // T: a → x = 5a
  // y mimics x with y = 2x + 1
  // Inputs: {a}. Outputs: {x, y}
  // Expected for a=3: x=15, y=31
  const auto joints = ensure_joints(analysis_, {"j_a", "j_x", "j_y"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});
  const auto y = ensure_state(analysis_, "j_y", InterfaceId{"position"});

  analysis_.add_affine_transmission(joints[1], joints[2], 2.0, 1.0);

  const auto model_id = analysis_.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{5.0}, std::vector<double>{0.0}, 1, 1));
  analysis_.add_transmission(model_id, {a}, {x}, "T");

  const auto jm = build_joint_map(analysis_, {a}, {x, y});

  std::vector<double> in{3.0};
  std::vector<double> out(2, 0.0);
  jm.map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 15.0));  // x = 5*3
  EXPECT_TRUE(approx_equal(out[1], 31.0));  // y = 2*15 + 1
}

// ===========================================================================
// Mixed: pre-transmission affine batch (mimic of input), then transmission
// ===========================================================================

TEST_F(MaterializeJointMapTest, Mixed_PreAffineBatchThenTransmission)
{
  // b mimics a (b = 2a + 5)
  // T: a → x = 3a + 1
  // Inputs: {a}. Outputs: {b, x}
  // Expected for a=4: b=13, x=13
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b", "j_x"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto x = ensure_state(analysis_, "j_x", InterfaceId{"position"});

  analysis_.add_affine_transmission(joints[0], joints[1], 2.0, 5.0);

  const auto model_id = analysis_.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{3.0}, std::vector<double>{1.0}, 1, 1));
  analysis_.add_transmission(model_id, {a}, {x}, "T");

  const auto jm = build_joint_map(analysis_, {a}, {b, x});

  std::vector<double> in{4.0};
  std::vector<double> out(2, 0.0);
  jm.map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 13.0));  // 2*4 + 5
  EXPECT_TRUE(approx_equal(out[1], 13.0));  // 3*4 + 1
}

// ===========================================================================
// Mixed: side-effect output (transmission produces 2 outputs, only 1 wanted)
// ===========================================================================

TEST_F(MaterializeJointMapTest, Mixed_SideEffectOutput_OnlyWantedExposed)
{
  // T: a → {i, k}, where i = 2a, k = 3a + 1
  // Inputs: {a}. Outputs: {k}
  // Expected for a=5: k=16
  ensure_joints(analysis_, {"j_a", "j_i", "j_k"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto i_iface = ensure_state(analysis_, "j_i", InterfaceId{"position"});
  const auto k = ensure_state(analysis_, "j_k", InterfaceId{"position"});

  // 2-output linear: outputs = [2a, 3a+1]
  const auto model_id = analysis_.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{2.0, 3.0},
    std::vector<double>{0.0, 1.0},
    1, 2));
  analysis_.add_transmission(model_id, {a}, {i_iface, k}, "T");

  const auto jm = build_joint_map(analysis_, {a}, {k});

  std::vector<double> in{5.0};
  std::vector<double> out(1, 0.0);
  jm.map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 16.0));
}

// ===========================================================================
// Mixed: chain of two transmissions (T1: a → b; T2: b → c)
// ===========================================================================

TEST_F(MaterializeJointMapTest, Mixed_TransmissionChain)
{
  // T1: a → b = 2a + 1
  // T2: b → c = 3b - 2
  // Inputs: {a}. Outputs: {c}
  // For a=4: b=9, c=25
  ensure_joints(analysis_, {"j_a", "j_b", "j_c"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto c = ensure_state(analysis_, "j_c", InterfaceId{"position"});

  const auto m1 = analysis_.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{2.0}, std::vector<double>{1.0}, 1, 1));
  const auto m2 = analysis_.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{3.0}, std::vector<double>{-2.0}, 1, 1));
  analysis_.add_transmission(m1, {a}, {b}, "T1");
  analysis_.add_transmission(m2, {b}, {c}, "T2");

  const auto jm = build_joint_map(analysis_, {a}, {c});

  std::vector<double> in{4.0};
  std::vector<double> out(1, 0.0);
  jm.map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 25.0));
}

// ===========================================================================
// Mixed: transmission output is also exposed AS a wanted output AND used downstream
// ===========================================================================

TEST_F(MaterializeJointMapTest, Mixed_TransmissionOutputBothExposedAndConsumedDownstream)
{
  // T: a → b = 2a
  // y mimics b with y = 3b
  // Inputs: {a}. Outputs: {b, y}
  // For a=5: b=10, y=30
  // The transmission scatters b to scratch (so y can read it) AND to the output buffer
  // (so the caller sees it).
  const auto joints = ensure_joints(analysis_, {"j_a", "j_b", "j_y"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});
  const auto b = ensure_state(analysis_, "j_b", InterfaceId{"position"});
  const auto y = ensure_state(analysis_, "j_y", InterfaceId{"position"});

  analysis_.add_affine_transmission(joints[1], joints[2], 3.0, 0.0);

  const auto model_id = analysis_.add_model(std::make_unique<LinearTransmissionModel>(
    std::vector<double>{2.0}, std::vector<double>{0.0}, 1, 1));
  analysis_.add_transmission(model_id, {a}, {b}, "T");

  const auto jm = build_joint_map(analysis_, {a}, {b, y});

  std::vector<double> in{5.0};
  std::vector<double> out(2, 0.0);
  jm.map(in, out);
  EXPECT_TRUE(approx_equal(out[0], 10.0));
  EXPECT_TRUE(approx_equal(out[1], 30.0));
}

// ===========================================================================
// Empty output request
// ===========================================================================

TEST_F(MaterializeJointMapTest, EmptyOutputs_ProducesValidEmptyJointMap)
{
  ensure_joints(analysis_, {"j_a"});
  const auto a = ensure_state(analysis_, "j_a", InterfaceId{"position"});

  const auto jm = build_joint_map(analysis_, {a}, {});

  EXPECT_EQ(jm.output_count(), 0u);

  std::vector<double> in{1.0};
  std::vector<double> out{};
  jm.map(in, out);  // No-op, should not throw
}
