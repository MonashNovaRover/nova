#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "arm_kinematics/collision/collision_manager.hpp"
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
