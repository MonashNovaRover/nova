// Created by Codex on 21/04/2026.

#include <gtest/gtest.h>

#include <rclcpp/rclcpp.hpp>
#include <urdf_model/model.h>

#include <functional>
#include <utility>
#include <vector>

#include "arm_kinematics/collision/collision_manager.hpp"

namespace arm_kinematics {
namespace {

class FakeTree final : public ForwardKinematicsPlugin::Tree {
public:
  FakeTree()
  : Tree(1)
  {
  }

  void position_fk(span<const double> joint_states, Isometry3dVector & link_poses) override
  {
    if (joint_states.size() != 1 || link_poses.size() != 1) {
      throw std::invalid_argument("FakeTree expects one input joint and one output pose");
    }

    link_poses[0] = Eigen::Isometry3d::Identity();
    link_poses[0].translation().x() = joint_states[0];
  }
};

class FakeCollisionPlugin final : public DiscreteCollisionPlugin {
public:
  explicit FakeCollisionPlugin(std::function<bool(double)> predicate)
  : predicate_(std::move(predicate))
  {
  }

  void update_pose(size_t idx, const Eigen::Isometry3d & collider_pose) override
  {
    if (idx != 0) {
      throw std::invalid_argument("FakeCollisionPlugin only models one collider");
    }
    last_x_ = collider_pose.translation().x();
    seen_positions_.push_back(last_x_);
  }

  bool collide() override
  {
    return predicate_(last_x_);
  }

  bool collide(
    std::vector<std::pair<size_t, size_t>> & colliding_pairs,
    size_t max_colliding_pairs) override
  {
    const bool collides = collide();
    if (collides && max_colliding_pairs > 0) {
      colliding_pairs.emplace_back(0, 0);
    }
    return collides;
  }

  const std::vector<double> & seen_positions() const noexcept
  {
    return seen_positions_;
  }

protected:
  bool on_initialize(
    const std::vector<std::reference_wrapper<const urdf::Collision>> & collider_geometries) override
  {
    return collider_geometries.size() == 1;
  }

private:
  std::function<bool(double)> predicate_;
  double last_x_{0.0};
  std::vector<double> seen_positions_{};
};

CollisionManager make_fake_manager(const std::shared_ptr<FakeCollisionPlugin> & plugin)
{
  auto node = std::make_shared<rclcpp::Node>("test_collision_path");
  urdf::Collision collision;
  std::vector<std::reference_wrapper<const urdf::Collision>> geometries{std::cref(collision)};
  EXPECT_TRUE(plugin->initialize(*node, geometries, AllowedCollisionMatrix(1)));

  return CollisionManager{
    std::make_shared<FakeTree>(),
    plugin,
  };
}

TEST(CollisionPathTest, ReturnsFalseWhenEndpointIsSafe)
{
  auto plugin = std::make_shared<FakeCollisionPlugin>(
    [](double x) { return x >= 1.0; });
  auto manager = make_fake_manager(plugin);

  const auto result = check_path_collision(
    manager,
    std::vector<double>{0.0},
    std::vector<double>{0.4},
    0.2);
  ASSERT_TRUE(result);
  EXPECT_FALSE(*result);
}

TEST(CollisionPathTest, ReturnsTrueWhenEndpointCollides)
{
  auto plugin = std::make_shared<FakeCollisionPlugin>(
    [](double x) { return x >= 1.0; });
  auto manager = make_fake_manager(plugin);

  const auto result = check_path_collision(
    manager,
    std::vector<double>{0.0},
    std::vector<double>{1.0},
    2.0);
  ASSERT_TRUE(result);
  EXPECT_TRUE(*result);
}

TEST(CollisionPathTest, ReturnsTrueWhenOnlyIntermediateStateCollides)
{
  auto plugin = std::make_shared<FakeCollisionPlugin>(
    [](double x) { return x >= 0.35 && x <= 0.45; });
  auto manager = make_fake_manager(plugin);

  const auto result = check_path_collision(
    manager,
    std::vector<double>{0.0},
    std::vector<double>{0.6},
    0.25);
  ASSERT_TRUE(result);
  EXPECT_TRUE(*result);
}

TEST(CollisionPathTest, UsesExpectedStepsForNonMultipleDisplacement)
{
  auto plugin = std::make_shared<FakeCollisionPlugin>(
    [](double) { return false; });
  auto manager = make_fake_manager(plugin);

  const auto result = check_path_collision(
    manager,
    std::vector<double>{0.0},
    std::vector<double>{0.6},
    0.25);
  ASSERT_TRUE(result);
  EXPECT_FALSE(*result);
  ASSERT_EQ(plugin->seen_positions().size(), 3u);
  EXPECT_NEAR(plugin->seen_positions()[0], 0.2, 1.0e-12);
  EXPECT_NEAR(plugin->seen_positions()[1], 0.4, 1.0e-12);
  EXPECT_NEAR(plugin->seen_positions()[2], 0.6, 1.0e-12);
}

TEST(CollisionPathTest, UsesExpectedStepsForExactMultipleDisplacement)
{
  auto plugin = std::make_shared<FakeCollisionPlugin>(
    [](double) { return false; });
  auto manager = make_fake_manager(plugin);

  const auto result = check_path_collision(
    manager,
    std::vector<double>{0.0},
    std::vector<double>{0.75},
    0.25);
  ASSERT_TRUE(result);
  EXPECT_FALSE(*result);
  ASSERT_EQ(plugin->seen_positions().size(), 3u);
  EXPECT_DOUBLE_EQ(plugin->seen_positions()[0], 0.25);
  EXPECT_DOUBLE_EQ(plugin->seen_positions()[1], 0.5);
  EXPECT_DOUBLE_EQ(plugin->seen_positions()[2], 0.75);
}

TEST(CollisionPathTest, ReusesCallerOwnedScratch)
{
  auto plugin = std::make_shared<FakeCollisionPlugin>(
    [](double) { return false; });
  auto manager = make_fake_manager(plugin);
  std::vector<double> scratch;
  scratch.assign(1, -1.0);

  const auto result = check_path_collision(
    manager,
    std::vector<double>{0.0},
    std::vector<double>{0.75},
    0.25,
    scratch);
  ASSERT_TRUE(result);
  EXPECT_FALSE(*result);
}

TEST(CollisionPathTest, ReturnsStructuredErrorOnMismatchedJointVectorSizes)
{
  auto plugin = std::make_shared<FakeCollisionPlugin>(
    [](double) { return false; });
  auto manager = make_fake_manager(plugin);

  const auto result = check_path_collision(
    manager,
    std::vector<double>{0.0},
    std::vector<double>{0.0, 1.0},
    0.25);
  ASSERT_FALSE(result);
  EXPECT_EQ(
    result.error().format(),
    "check_path_collision() received start/end vectors with different sizes (1 vs 2)");
}

TEST(CollisionPathTest, ReturnsStructuredErrorOnNonPositiveStepSize)
{
  auto plugin = std::make_shared<FakeCollisionPlugin>(
    [](double) { return false; });
  auto manager = make_fake_manager(plugin);

  const auto result = check_path_collision(
    manager,
    std::vector<double>{0.0},
    std::vector<double>{1.0},
    0.0);
  ASSERT_FALSE(result);
  EXPECT_EQ(
    result.error().format(),
    "check_path_collision() requires step_size > 0, got 0.000000");
}

}  // namespace
}  // namespace arm_kinematics
