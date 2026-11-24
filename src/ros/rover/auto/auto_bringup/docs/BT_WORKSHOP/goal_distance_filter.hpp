// Copyright (c) 2025 Monash Nova Rover
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef NOVA_BEHAVIOR_TREE__ACTION__GOAL_DISTANCE_FILTER_HPP_
#define NOVA_BEHAVIOR_TREE__ACTION__GOAL_DISTANCE_FILTER_HPP_

#include <memory>
#include <string>

#include "behaviortree_cpp/action_node.h"
#include "behaviortree_cpp/bt_factory.h"
#include "geometry_msgs/msg/point.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "tf2_ros/buffer.h"

namespace nova_behavior_tree
{

/**
 * @brief Removes the navigation goal from the blackboard if it is too far away.
 *
 * Designed for NavigateToPose, so it only ever deals with a single goal.
 *
 * Ports:
 *   - input  "goal"         (geometry_msgs::msg::PoseStamped) goal to check
 *   - input  "max_distance" (double, metres) furthest allowed goal distance
 *   - output "distance"     (double, metres) measured robot-to-goal distance
 *
 * Returns SUCCESS if the goal is within max_distance (the goal is left alone).
 * Returns FAILURE if the goal is too far (the goal is removed from the
 * blackboard) or if an input could not be read or the distance could not be
 * determined (the goal is kept).
 */
class GoalDistanceFilter : public BT::SyncActionNode
{
public:
  GoalDistanceFilter(
    const std::string & name,
    const BT::NodeConfiguration & config);

  // Declares the node's ports. BT.CPP calls this when registering the node.
  static BT::PortsList providedPorts();

  // Called every time the node is ticked. Must return SUCCESS or FAILURE
  // (a SyncActionNode can never return RUNNING).
  BT::NodeStatus tick() override;

  // 2D (x, y) distance between two points.
  // Public and static so it can be unit-tested without building a tree.
  static double computeDistance(
    const geometry_msgs::msg::Point & a,
    const geometry_msgs::msg::Point & b);

private:
  // Provided helper (not part of the exercise): looks up the robot's position
  // in the given frame using TF. Returns false if the lookup fails.
  bool getRobotPosition(
    const std::string & frame,
    geometry_msgs::msg::Point & position);

  // Provided helper (not part of the exercise): removes the blackboard entry
  // that the "goal" port is mapped to.
  void removeGoal();

  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
};

}  // namespace nova_behavior_tree

#endif  // NOVA_BEHAVIOR_TREE__ACTION__GOAL_DISTANCE_FILTER_HPP_
