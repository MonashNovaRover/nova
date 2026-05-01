//
// Created by Bailey Chessum on 19/04/2026.
//

#include <gtest/gtest.h>

#include <rclcpp/rclcpp.hpp>

#include "arm_kinematics/collision/collision_utilities.hpp"

namespace arm_kinematics {
namespace {

TEST(CollisionUtilitiesTest, AllowCollisionPairsByLinkAllowsAllColliderCrossProducts)
{
  AllowedCollisionMatrix acm(5);
  const std::vector<std::string> parent_link_names{
    "base_link",
    "shoulder_link",
    "shoulder_link",
    "wrist_link",
    "tool_link",
  };
  const std::vector<std::pair<std::string, std::string>> allowed_pairs{
    {"base_link", "shoulder_link"},
    {"wrist_link", "tool_link"},
  };

  allow_collision_pairs_by_link(
    acm,
    {parent_link_names.data(), parent_link_names.size()},
    {allowed_pairs.data(), allowed_pairs.size()});

  EXPECT_TRUE(acm.get(0, 1));
  EXPECT_TRUE(acm.get(0, 2));
  EXPECT_TRUE(acm.get(3, 4));
  EXPECT_FALSE(acm.get(1, 3));
}

TEST(CollisionUtilitiesTest, ReadCollisionConfigParsesValidEntriesAndSkipsMalformedOnes)
{
  auto node = std::make_shared<rclcpp::Node>("test_collision_config");
  node->declare_parameter("collision.generate_from_default_pose", false);
  node->declare_parameter(
    "collision.default_pose_overrides",
    std::vector<std::string>{" shoulder = 0.25 ", "bad", "shoulder=0.5", "elbow=-1.25"});
  node->declare_parameter(
    "collision.allowed_pairs",
    std::vector<std::string>{" base_link, shoulder_link ", "broken", "wrist_link,tool_link"});

  const auto config = read_collision_config(node->get_node_parameters_interface());

  EXPECT_FALSE(config.generate_from_default_pose);
  ASSERT_EQ(config.default_pose_overrides.size(), 2u);
  EXPECT_DOUBLE_EQ(config.default_pose_overrides.at("shoulder"), 0.5);
  EXPECT_DOUBLE_EQ(config.default_pose_overrides.at("elbow"), -1.25);
  ASSERT_EQ(config.allowed_pairs.size(), 2u);
  EXPECT_EQ(config.allowed_pairs[0], std::make_pair(std::string("base_link"), std::string("shoulder_link")));
  EXPECT_EQ(config.allowed_pairs[1], std::make_pair(std::string("wrist_link"), std::string("tool_link")));
}

}  // namespace
}  // namespace arm_kinematics
