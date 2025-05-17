// Copyright (c) 2021 Samsung Research America
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

#include <string>
#include <memory>
#include <limits>
#include <vector>
#include "nav2_util/geometry_utils.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "rclcpp/logging.hpp"
#include "nova_behavior_tree/action/update_urc_goals_action.hpp"

namespace nova_behavior_tree
{

  UpdateURCGoalsAction::UpdateURCGoalsAction(
  const std::string & name,
  const BT::NodeConfiguration & conf)
  : BT::ActionNodeBase(name, conf)
  {
  }

  void UpdateURCGoalsAction::initialize()
  {
    node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
    
    getInput("max_update_radius", max_update_radius_);
    
    initialized_ = true;
  }

  inline BT::NodeStatus UpdateURCGoalsAction::tick()
  {
    // 📝 Initialise the node on startup with static inputs
    if (!initialized_)
    {
        initialize();
    }

    // 📝 Get new dynamic inputs
    Goals goals;
    getInput("input_goals", goals);
    geometry_msgs::msg::PoseStamped goal;
    getInput("new_goal", goal);

    // 📝 Calculate distance between the new goal and the current goal
    using namespace nav2_util::geometry_utils;  // NOLINT

    double dist_between_goals = 0.0;
    if (!goals.empty()) 
    {
      dist_between_goals = euclidean_distance(goals.back().pose, goal.pose);
    }

    // 📝 If the distance between the new goal and the current goal is within max_update_radius_, update the current goal with the new goal's pose.
    if (dist_between_goals < max_update_radius_) 
    {
      goals.back().pose = goal.pose;
      setOutput("output_goals", goals);
      RCLCPP_INFO(node_->get_logger(), "Updating goal pose");
    }

    return BT::NodeStatus::SUCCESS;
  }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::UpdateURCGoalsAction>("UpdateURCGoals");
}