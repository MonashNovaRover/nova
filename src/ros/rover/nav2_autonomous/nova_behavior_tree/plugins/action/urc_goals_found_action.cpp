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
#include "nova_behavior_tree/action/urc_goals_found_action.hpp"

namespace nova_behavior_tree
{

  URCGoalsFoundAction::URCGoalsFoundAction(
  const std::string & name,
  const BT::NodeConfiguration & conf)
  : BT::ActionNodeBase(name, conf)
  {
  }

  void URCGoalsFoundAction::initialize()
  {
    node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
    getInput("max_time", goal_max_time_);
    getInput("min_number", goal_min_number_);
    
    initialized_ = true;
  }

  inline BT::NodeStatus URCGoalsFoundAction::tick()
  {
    // 📝 Initialise the node on startup with static inputs
    if (!initialized_)
    {
      initialize();
    }

    // 📝 Get new dynamic inputs
    int id;
    geometry_msgs::msg::PoseStamped goal;
    getInput("id", id);
    getInput("goal", goal);

    // 📝 Determine if a goal has been found
    if (!goal_poses_.empty()) 
    {
      // 📝 If the new goal was detected within goal_max_time_ of the old goal, add it to detected same goals
      if (
        ((goal.header.stamp.sec - goal_poses_.back().header.stamp.sec) < goal_max_time_) &&
        (id == goal_id_)
      )
      {
        goal_poses_.push_back(goal);
      }
      else 
      {
        goal_poses_.clear();
      }

      // 📝 If goal_min_number_ same goals have been detected, the goal is valid and has been found
      if (goal_poses_.size() >= goal_min_number_)
      {
        Goals goal_poses;
        goal_poses.push_back(goal_poses_.back());
        goal_poses_.clear();
        goal_found_ = 1;
        setOutput("output_goals", goal_poses);
      }
    }
    else 
    {
      goal_poses_.push_back(goal);
      goal_id_ = id;
    }

    setOutput("found", goal_found_);

    return BT::NodeStatus::SUCCESS;
  }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::URCGoalsFoundAction>("URCGoalsFound");
}