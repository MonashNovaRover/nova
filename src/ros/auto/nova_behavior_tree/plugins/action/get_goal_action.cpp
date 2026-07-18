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

/**
 * @brief Extracts a goal from a goals vector based on the given index.
 * 
 * @authors Terry Tian
 */

#include <string>
#include <cmath>

#include "rclcpp/logging.hpp"

#include "nova_behavior_tree/action/get_goal_action.hpp"

namespace nova_behavior_tree
{

    GetGoalAction::GetGoalAction(
    const std::string & name,
    const BT::NodeConfiguration & conf)
    : BT::ActionNodeBase(name, conf)
    {
    }

    void GetGoalAction::initialize()
    {
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
        initialized_ = true;
    }
    
    inline BT::NodeStatus GetGoalAction::tick()
    {
        if (!initialized_)
        {
            initialize();
        }
        
        getInput("index", index_);
        getInput("input_goals", input_goals_);
        
        if (get_goal())
        {
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::FAILURE;
    }

    bool GetGoalAction::get_goal()
    {
        if (index_ > static_cast<int>(input_goals_.size()) - 1 ||
            std::abs(index_) > static_cast<int>(input_goals_.size()))
        {
            RCLCPP_ERROR(node_->get_logger(), "Index out of bounds");
            return false;
        }
        
        if (index_ >= 0)
        {
            setOutput("output_goal", input_goals_[index_]);
        }
        else
        {
            setOutput("output_goal", input_goals_[input_goals_.size() + index_]);
        }
        return true;
    }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::GetGoalAction>("GetGoal");
}