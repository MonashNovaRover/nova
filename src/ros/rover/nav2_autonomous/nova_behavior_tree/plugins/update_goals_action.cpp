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

/**
 * @brief Action node for updating/inserting goals. Goals are inserted based
 * on their distance to the rover, with the closest goals at the front and
 * furthest at the back.
 * 
 * @authors Terry Tian
 */

#include <string>
#include <memory>
#include <limits>
#include <vector>

#include "nav2_util/geometry_utils.hpp"
#include "rclcpp/logging.hpp"

#include "nova_behavior_tree/update_goals_action.hpp"
#include "nova_behavior_tree/nav2_utils.hpp"

namespace nova_behavior_tree
{

    using namespace nav2_util::geometry_utils;

    UpdateGoalsAction::UpdateGoalsAction(
    const std::string & name,
    const BT::NodeConfiguration & conf)
    : BT::ActionNodeBase(name, conf),
    viapoint_overwrite_tolerance_(0.5)
    {
    }

    void UpdateGoalsAction::initialize()
    {
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
        
        getInput("radius", viapoint_overwrite_tolerance_);
        getInput("goal_type", goal_type_);
        
        initialized_ = true;
    }

    inline BT::NodeStatus UpdateGoalsAction::tick()
    {
        if (!initialized_)
        {
            initialize();
        }
        
        getInput("current_pose", current_pose_);
        getInput("goals", goals_);
        getInput("input_goals", input_goals_);

        for (auto goal : goals_)
        {
            if (utils::nav2::isDefaultPose(goal.pose))
            {
                continue;
            }

            double dist_to_rover = euclidean_distance(current_pose_.pose, goal.pose);

            size_t i = 0;
            while (i < input_goals_.size() && 
                   dist_to_rover > euclidean_distance(current_pose_.pose, input_goals_[i].pose))
            {
                i += 1;
            }

            if (i > 0 &&
                euclidean_distance(input_goals_[i - 1].pose, goal.pose) < viapoint_overwrite_tolerance_)
            {
                input_goals_[i - 1].pose = goal.pose;
                RCLCPP_INFO(node_->get_logger(), "Updating existing %s goal", goal_type_.c_str());
            }
            else if (i < input_goals_.size() - 1 &&
                  euclidean_distance(input_goals_[i + 1].pose, goal.pose) < viapoint_overwrite_tolerance_)
            {
                input_goals_[i + 1].pose = goal.pose;
                RCLCPP_INFO(node_->get_logger(), "Updating existing %s goal", goal_type_.c_str());
            }
            else
            {
                input_goals_.insert(input_goals_.begin() + i, goal);
                RCLCPP_INFO(node_->get_logger(), "Inserting new %s goal", goal_type_.c_str());
            }
        }

        setOutput("output_goals", input_goals_);

        return BT::NodeStatus::SUCCESS;
    }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::UpdateGoalsAction>("UpdateGoals");
}