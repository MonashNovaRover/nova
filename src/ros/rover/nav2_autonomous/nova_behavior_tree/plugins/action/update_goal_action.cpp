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

#include "nova_behavior_tree/action/update_goal_action.hpp"

namespace nova_behavior_tree
{

    UpdateGoalAction::UpdateGoalAction(
    const std::string & name,
    const BT::NodeConfiguration & conf)
    : BT::ActionNodeBase(name, conf),
    viapoint_overwrite_tolerance_(0.5)
    {
    }

    void UpdateGoalAction::initialize()
    {
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
        
        getInput("radius", viapoint_overwrite_tolerance_);
        getInput("goal_type", goal_type_);
        
        initialized_ = true;
    }

    inline BT::NodeStatus UpdateGoalAction::tick()
    {
        if (!initialized_)
        {
            initialize();
        }
        
        Goals goal_poses;
        getInput("input_goals", goal_poses);
        geometry_msgs::msg::PoseStamped goal;
        getInput("input_goal", goal);

        using namespace nav2_util::geometry_utils;  // NOLINT

        double dist_between_goals = 0.0;
        if (!goal_poses.empty()) 
        {
            dist_between_goals = euclidean_distance(goal_poses[0].pose, goal.pose);
        }

        if (dist_between_goals < viapoint_overwrite_tolerance_) 
        {
            goal_poses[0].pose = goal.pose;
            RCLCPP_INFO(node_->get_logger(), "Updating existing %s goal", goal_type_.c_str());
        }
        else 
        {
            goal_poses.insert(goal_poses.begin(), goal);
            RCLCPP_INFO(node_->get_logger(), "Inserting new %s goal", goal_type_.c_str());
        }

        setOutput("output_goals", goal_poses);

        return BT::NodeStatus::SUCCESS;
    }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::UpdateGoalAction>("UpdateGoal");
}