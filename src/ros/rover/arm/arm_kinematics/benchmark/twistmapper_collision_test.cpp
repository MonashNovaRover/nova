#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "arm_kinematics/collision/collision_manager.hpp"
#include "arm_kinematics/plugins/forward/default_forward_kinematics_plugin.hpp"
#include "twistmapper_benchmark_support.hpp"

namespace {

using arm_kinematics_benchmark::TwistmapperCollisionRuntime;

struct CollisionCheckResult {
  bool collided{false};
  std::set<std::string> named_pairs;
};

CollisionCheckResult direct_collision(
  TwistmapperCollisionRuntime & runtime,
  const std::vector<double> & state)
{
  runtime.collision_manager.update_poses(state);
  runtime.colliding_pairs_scratch.clear();

  CollisionCheckResult result;
  result.collided = runtime.collision_manager.collide(runtime.colliding_pairs_scratch);
  for (const auto & [a, b] : runtime.colliding_pairs_scratch) {
    const std::string & lhs = runtime.collision_manager.parent_link_names().at(a);
    const std::string & rhs = runtime.collision_manager.parent_link_names().at(b);
    if (lhs < rhs) {
      result.named_pairs.insert(lhs + "|" + rhs);
    } else {
      result.named_pairs.insert(rhs + "|" + lhs);
    }
  }
  return result;
}

CollisionCheckResult path_collision(
  TwistmapperCollisionRuntime & runtime,
  const std::vector<double> & start,
  const std::vector<double> & end)
{
  auto collision_result = arm_kinematics::check_path_collision(
    runtime.collision_manager,
    start,
    end,
    arm_kinematics_benchmark::kSelfIntersectionMaxStepSize,
    runtime.joint_values_scratch,
    runtime.colliding_pairs_scratch);
  if (!collision_result) {
    throw std::runtime_error(collision_result.error().format());
  }

  CollisionCheckResult result;
  result.collided = *collision_result;
  for (const auto & [a, b] : runtime.colliding_pairs_scratch) {
    const std::string & lhs = runtime.collision_manager.parent_link_names().at(a);
    const std::string & rhs = runtime.collision_manager.parent_link_names().at(b);
    if (lhs < rhs) {
      result.named_pairs.insert(lhs + "|" + rhs);
    } else {
      result.named_pairs.insert(rhs + "|" + lhs);
    }
  }
  return result;
}

std::string format_state(const std::vector<double> & state)
{
  std::ostringstream stream;
  stream << "[";
  for (std::size_t i = 0; i < state.size(); ++i) {
    if (i != 0) {
      stream << ", ";
    }
    stream << state[i];
  }
  stream << "]";
  return stream.str();
}

std::string format_pairs(const std::set<std::string> & pairs)
{
  std::ostringstream stream;
  stream << "{";
  bool first = true;
  for (const auto & pair : pairs) {
    if (!first) {
      stream << ", ";
    }
    first = false;
    stream << pair;
  }
  stream << "}";
  return stream.str();
}

std::string format_values(const std::vector<double> & values)
{
  std::ostringstream stream;
  stream << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      stream << ", ";
    }
    stream << values[i];
  }
  stream << "]";
  return stream.str();
}

std::string format_pose(const Eigen::Isometry3d & pose)
{
  std::ostringstream stream;
  const auto & matrix = pose.matrix();
  stream << "[";
  for (Eigen::Index row = 0; row < matrix.rows(); ++row) {
    if (row != 0) {
      stream << "; ";
    }
    stream << "[";
    for (Eigen::Index col = 0; col < matrix.cols(); ++col) {
      if (col != 0) {
        stream << ", ";
      }
      stream << matrix(row, col);
    }
    stream << "]";
  }
  stream << "]";
  return stream.str();
}

std::string format_poses(const arm_kinematics::Isometry3dVector & poses)
{
  std::ostringstream stream;
  stream << "[";
  for (std::size_t i = 0; i < poses.size(); ++i) {
    if (i != 0) {
      stream << ", ";
    }
    stream << format_pose(poses[i]);
  }
  stream << "]";
  return stream.str();
}

bool poses_exactly_equal(
  const arm_kinematics::Isometry3dVector & lhs,
  const arm_kinematics::Isometry3dVector & rhs)
{
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (!lhs[i].matrix().isApprox(rhs[i].matrix(), 0.0)) {
      return false;
    }
  }

  return true;
}

const std::vector<std::vector<double>> & search_states()
{
  static const std::vector<std::vector<double>> states = [] {
    std::vector<std::vector<double>> result{
      {0.0, -0.35, 0.4, 0.1, -0.25, 0.2},
      {0.08, -0.22, 0.55, 0.18, -0.1, 0.28},
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      {0.0, -1.0, 1.0, 0.0, 0.0, 0.0},
      {0.0, -1.2, 1.2, 0.0, 0.0, 0.0},
      {0.0, -1.2, 0.6, -0.6, 0.0, 0.0},
    };

    constexpr std::array<double, 5> samples{-1.2, -0.6, 0.0, 0.6, 1.2};
    for (const double j1 : samples) {
      for (const double j2 : samples) {
        for (const double j3 : samples) {
          for (const double j4 : samples) {
            for (const double j5 : samples) {
              for (const double j6 : samples) {
                result.push_back({j1, j2, j3, j4, j5, j6});
              }
            }
          }
        }
      }
    }

    return result;
  }();
  return states;
}

const std::vector<double> & find_state(const bool expect_collision)
{
  static const std::vector<double> safe_state = [] {
    TwistmapperCollisionRuntime runtime("tm_collision_safe_state_search");
    for (const auto & state : search_states()) {
      if (!direct_collision(runtime, state).collided) {
        return state;
      }
    }
    throw std::runtime_error("Failed to find a safe twistmapper collision test state.");
  }();

  static const std::vector<double> colliding_state = [] {
    TwistmapperCollisionRuntime runtime("tm_collision_colliding_state_search");
    for (const auto & state : search_states()) {
      const auto first = direct_collision(runtime, state);
      const auto second = direct_collision(runtime, state);
      if (first.collided && second.collided && first.named_pairs == second.named_pairs) {
        return state;
      }
    }
    throw std::runtime_error("Failed to find a stable colliding twistmapper collision test state.");
  }();

  return expect_collision ? colliding_state : safe_state;
}

const std::vector<double> & find_distinct_safe_state()
{
  static const std::vector<double> state = [] {
    const auto & primary_safe = find_state(false);
    TwistmapperCollisionRuntime runtime("tm_collision_safe_transition_state_search");
    for (const auto & candidate : search_states()) {
      if (candidate == primary_safe) {
        continue;
      }
      if (!direct_collision(runtime, candidate).collided) {
        return candidate;
      }
    }
    throw std::runtime_error("Failed to find a second safe twistmapper collision test state.");
  }();

  return state;
}

}  // namespace

TEST(TwistmapperCollisionDiagnosticTest, StationarySafePathReportsNoCollision)
{
  TwistmapperCollisionRuntime runtime("tm_collision_stationary_safe");
  const auto & safe_state = find_state(false);

  const auto result = path_collision(runtime, safe_state, safe_state);

  EXPECT_FALSE(result.collided) << format_state(safe_state);
  EXPECT_TRUE(result.named_pairs.empty());
  EXPECT_TRUE(runtime.colliding_pairs_scratch.empty());
}

TEST(TwistmapperCollisionDiagnosticTest, StationarySafeVelocityPredictionReportsNoCollision)
{
  TwistmapperCollisionRuntime runtime("tm_collision_stationary_velocity_safe");
  const auto & safe_state = find_state(false);
  runtime.current_joint_state_values = safe_state;
  runtime.predicted_joint_positions = safe_state;

  const auto result = path_collision(
    runtime,
    runtime.current_joint_state_values,
    runtime.predicted_joint_positions);

  EXPECT_FALSE(result.collided) << format_state(safe_state);
  EXPECT_TRUE(result.named_pairs.empty());
}

TEST(TwistmapperCollisionDiagnosticTest, RepeatedStationarySafeChecksRemainStable)
{
  TwistmapperCollisionRuntime runtime("tm_collision_stationary_repeat_safe");
  const auto & safe_state = find_state(false);

  const auto first = path_collision(runtime, safe_state, safe_state);
  const auto second = path_collision(runtime, safe_state, safe_state);
  const auto third = path_collision(runtime, safe_state, safe_state);

  EXPECT_FALSE(first.collided);
  EXPECT_FALSE(second.collided);
  EXPECT_FALSE(third.collided);
  EXPECT_EQ(first.named_pairs, second.named_pairs);
  EXPECT_EQ(second.named_pairs, third.named_pairs);
  EXPECT_TRUE(third.named_pairs.empty());
  EXPECT_TRUE(runtime.colliding_pairs_scratch.empty());
}

TEST(TwistmapperCollisionDiagnosticTest, StationaryCollidingStateReportsStablePairs)
{
  TwistmapperCollisionRuntime runtime("tm_collision_stationary_colliding");
  const auto & colliding_state = find_state(true);

  const auto first = path_collision(runtime, colliding_state, colliding_state);
  const auto second = path_collision(runtime, colliding_state, colliding_state);

  EXPECT_TRUE(first.collided) << format_state(colliding_state);
  EXPECT_TRUE(second.collided) << format_state(colliding_state);
  EXPECT_FALSE(first.named_pairs.empty());
  EXPECT_EQ(first.named_pairs, second.named_pairs);
}

TEST(TwistmapperCollisionDiagnosticTest, DirectCollisionAgreesWithStationaryPathCollision)
{
  TwistmapperCollisionRuntime runtime("tm_collision_direct_vs_path");

  for (const bool expect_collision : {false, true}) {
    const auto & state = find_state(expect_collision);
    const auto direct = direct_collision(runtime, state);
    const auto path = path_collision(runtime, state, state);

    EXPECT_EQ(direct.collided, path.collided) << format_state(state);
    EXPECT_EQ(direct.named_pairs, path.named_pairs) << format_state(state);
  }
}

TEST(TwistmapperCollisionDiagnosticTest, PositionModeStylePathChecksCoverSafeAndCollidingTargets)
{
  TwistmapperCollisionRuntime runtime("tm_collision_position_mode_path");
  const auto & start = find_state(false);
  const auto & safe_target = find_distinct_safe_state();
  const auto & colliding_target = find_state(true);

  const auto safe_path = path_collision(runtime, start, safe_target);
  const auto colliding_path = path_collision(runtime, start, colliding_target);

  EXPECT_FALSE(safe_path.collided)
    << "start=" << format_state(start) << " end=" << format_state(safe_target);
  EXPECT_TRUE(safe_path.named_pairs.empty());
  EXPECT_TRUE(colliding_path.collided)
    << "start=" << format_state(start) << " end=" << format_state(colliding_target);
  EXPECT_FALSE(colliding_path.named_pairs.empty());
}

TEST(TwistmapperCollisionDiagnosticTest, VelocityModeStylePathChecksCoverSafeAndCollidingPredictions)
{
  TwistmapperCollisionRuntime runtime("tm_collision_velocity_mode_path");
  const auto & start = find_state(false);
  const auto & safe_prediction = find_distinct_safe_state();
  const auto & colliding_prediction = find_state(true);

  runtime.current_joint_state_values = start;
  runtime.predicted_joint_positions = safe_prediction;
  const auto safe_path = path_collision(
    runtime,
    runtime.current_joint_state_values,
    runtime.predicted_joint_positions);

  runtime.predicted_joint_positions = colliding_prediction;
  const auto colliding_path = path_collision(
    runtime,
    runtime.current_joint_state_values,
    runtime.predicted_joint_positions);

  EXPECT_FALSE(safe_path.collided)
    << "start=" << format_state(start) << " predicted=" << format_state(safe_prediction);
  EXPECT_TRUE(safe_path.named_pairs.empty());
  EXPECT_TRUE(colliding_path.collided)
    << "start=" << format_state(start) << " predicted=" << format_state(colliding_prediction);
  EXPECT_FALSE(colliding_path.named_pairs.empty());
}

TEST(TwistmapperCollisionDiagnosticTest, RandomStationaryInputsRemainRepeatableOverManyRuns)
{
  TwistmapperCollisionRuntime runtime("tm_collision_random_repeatability");

  constexpr std::size_t kRandomStateCount = 25;
  constexpr std::size_t kRepeatCount = 101;
  constexpr double kLowerBound = -3.141592653589793;
  constexpr double kUpperBound = 3.141592653589793;

  std::mt19937 generator(0x5A17C0DEu);
  std::uniform_real_distribution<double> distribution(kLowerBound, kUpperBound);

  for (std::size_t state_index = 0; state_index < kRandomStateCount; ++state_index) {
    std::vector<double> state;
    state.reserve(arm_kinematics_benchmark::kJointNames.size());
    for (std::size_t joint_index = 0; joint_index < arm_kinematics_benchmark::kJointNames.size(); ++joint_index) {
      state.push_back(distribution(generator));
    }

    const auto expected = path_collision(runtime, state, state);
    for (std::size_t repeat_index = 1; repeat_index < kRepeatCount; ++repeat_index) {
      const auto actual = path_collision(runtime, state, state);
      EXPECT_EQ(actual.collided, expected.collided)
        << "state_index=" << state_index
        << " repeat_index=" << repeat_index
        << " state=" << format_state(state);
      EXPECT_EQ(actual.named_pairs, expected.named_pairs)
        << "state_index=" << state_index
        << " repeat_index=" << repeat_index
        << " state=" << format_state(state)
        << " expected_pairs=" << format_pairs(expected.named_pairs)
        << " actual_pairs=" << format_pairs(actual.named_pairs);
    }
  }
}

TEST(TwistmapperCollisionDiagnosticTest, RandomStationaryInputsKeepCollisionJointMapStableOverManyRuns)
{
  TwistmapperCollisionRuntime runtime("tm_collision_joint_map_repeatability");

  auto collision_result = runtime.loader.make_collision(
    arm_kinematics_benchmark::kJointNames,
    runtime.fk,
    arm_kinematics::span<const std::string>(
      arm_kinematics_benchmark::kIgnoredLinks.data(),
      arm_kinematics_benchmark::kIgnoredLinks.size()));
  ASSERT_TRUE(collision_result) << collision_result.error().format();

  const auto collision_frame_count = collision_result->parent_link_names.size();
  auto collision_tree = std::move(collision_result.value().fk_tree);
  auto * default_tree =
    dynamic_cast<arm_kinematics::DefaultForwardKinematicsPlugin::TreeImpl *>(collision_tree.get());
  ASSERT_NE(default_tree, nullptr);

  std::vector<double> expected_mapped_joint_states;
  arm_kinematics::Isometry3dVector expected_poses;
  arm_kinematics::Isometry3dVector scratch_poses(collision_frame_count);

  constexpr std::size_t kRandomStateCount = 25;
  constexpr std::size_t kRepeatCount = 101;
  constexpr double kLowerBound = -3.141592653589793;
  constexpr double kUpperBound = 3.141592653589793;

  std::mt19937 generator(0x4A019E2Du);
  std::uniform_real_distribution<double> distribution(kLowerBound, kUpperBound);

  for (std::size_t state_index = 0; state_index < kRandomStateCount; ++state_index) {
    std::vector<double> state;
    state.reserve(arm_kinematics_benchmark::kJointNames.size());
    for (std::size_t joint_index = 0; joint_index < arm_kinematics_benchmark::kJointNames.size(); ++joint_index) {
      state.push_back(distribution(generator));
    }

    default_tree->position_fk(state, scratch_poses);
    expected_mapped_joint_states = default_tree->get_mapped_joint_states();
    expected_poses = scratch_poses;

    for (std::size_t repeat_index = 1; repeat_index < kRepeatCount; ++repeat_index) {
      default_tree->position_fk(state, scratch_poses);
      EXPECT_EQ(default_tree->get_mapped_joint_states(), expected_mapped_joint_states)
        << "state_index=" << state_index
        << " repeat_index=" << repeat_index
        << " state=" << format_state(state)
        << " expected_mapped_joint_states=" << format_values(expected_mapped_joint_states)
        << " actual_mapped_joint_states=" << format_values(default_tree->get_mapped_joint_states());
      EXPECT_TRUE(poses_exactly_equal(scratch_poses, expected_poses))
        << "state_index=" << state_index
        << " repeat_index=" << repeat_index
        << " state=" << format_state(state)
        << " expected_poses=" << format_poses(expected_poses)
        << " actual_poses=" << format_poses(scratch_poses);
    }
  }
}

TEST(TwistmapperCollisionDiagnosticTest, RandomStationaryInputsKeepCollisionPluginCollideStableOverManyRuns)
{
  TwistmapperCollisionRuntime runtime("tm_collision_plugin_repeatability");

  auto collision_result = runtime.loader.make_collision(
    arm_kinematics_benchmark::kJointNames,
    runtime.fk,
    arm_kinematics::span<const std::string>(
      arm_kinematics_benchmark::kIgnoredLinks.data(),
      arm_kinematics_benchmark::kIgnoredLinks.size()));
  ASSERT_TRUE(collision_result) << collision_result.error().format();

  auto collision_plugin = std::move(collision_result.value().collision);
  ASSERT_NE(collision_plugin, nullptr);

  constexpr std::size_t kRandomStateCount = 25;
  constexpr std::size_t kRepeatCount = 101;
  constexpr double kLowerBound = -3.141592653589793;
  constexpr double kUpperBound = 3.141592653589793;

  std::mt19937 generator(0x2C0111DEu);
  std::uniform_real_distribution<double> distribution(kLowerBound, kUpperBound);

  for (std::size_t state_index = 0; state_index < kRandomStateCount; ++state_index) {
    std::vector<double> state;
    state.reserve(arm_kinematics_benchmark::kJointNames.size());
    for (std::size_t joint_index = 0; joint_index < arm_kinematics_benchmark::kJointNames.size(); ++joint_index) {
      state.push_back(distribution(generator));
    }

    runtime.collision_manager.update_poses(state);

    runtime.colliding_pairs_scratch.clear();
    const bool expected_collided = collision_plugin->collide(runtime.colliding_pairs_scratch);
    std::set<std::string> expected_pairs;
    for (const auto & [a, b] : runtime.colliding_pairs_scratch) {
      const std::string & lhs = runtime.collision_manager.parent_link_names().at(a);
      const std::string & rhs = runtime.collision_manager.parent_link_names().at(b);
      if (lhs < rhs) {
        expected_pairs.insert(lhs + "|" + rhs);
      } else {
        expected_pairs.insert(rhs + "|" + lhs);
      }
    }

    for (std::size_t repeat_index = 1; repeat_index < kRepeatCount; ++repeat_index) {
      runtime.colliding_pairs_scratch.clear();
      const bool actual_collided = collision_plugin->collide(runtime.colliding_pairs_scratch);
      std::set<std::string> actual_pairs;
      for (const auto & [a, b] : runtime.colliding_pairs_scratch) {
        const std::string & lhs = runtime.collision_manager.parent_link_names().at(a);
        const std::string & rhs = runtime.collision_manager.parent_link_names().at(b);
        if (lhs < rhs) {
          actual_pairs.insert(lhs + "|" + rhs);
        } else {
          actual_pairs.insert(rhs + "|" + lhs);
        }
      }

      EXPECT_EQ(actual_collided, expected_collided)
        << "state_index=" << state_index
        << " repeat_index=" << repeat_index
        << " state=" << format_state(state);
      EXPECT_EQ(actual_pairs, expected_pairs)
        << "state_index=" << state_index
        << " repeat_index=" << repeat_index
        << " state=" << format_state(state)
        << " expected_pairs=" << format_pairs(expected_pairs)
        << " actual_pairs=" << format_pairs(actual_pairs);
    }
  }
}
