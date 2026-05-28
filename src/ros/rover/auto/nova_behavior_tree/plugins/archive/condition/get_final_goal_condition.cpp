// Copyright (c) 2019 Intel Corporation
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

#include <rclcpp/logging.hpp>
#include <string>

#include "nova_behavior_tree/get_final_goal_condition.hpp"

namespace nova_behavior_tree
{

    GetFinalGoalCondition::GetFinalGoalCondition(
        const std::string &condition_name,
        const BT::NodeConfiguration &conf)
        : BT::ConditionNode(condition_name, conf),
          initialized_(false)
    {
    }

    GetFinalGoalCondition::~GetFinalGoalCondition()
    {
        cleanup();
    }

    void GetFinalGoalCondition::initialize()
    {
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");

        initialized_ = true;
    }

    BT::NodeStatus GetFinalGoalCondition::tick()
    {
        if (!initialized_)
        {
            initialize();
        }

        if (getFinalGoal())
        {
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::FAILURE;
    }

    bool GetFinalGoalCondition::getFinalGoal()
    {
        std::vector<geometry_msgs::msg::PoseStamped> goals;
        geometry_msgs::msg::PoseStamped final_goal;
        getInput("goals", goals);
        if (goals.size() == 0) {
            RCLCPP_ERROR(node_->get_logger(), "Empty goals list.");
            return false;
        }
        final_goal = goals.back();

        setOutput("final_goal", final_goal);
        return true;
    }

} // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
    factory.registerNodeType<nova_behavior_tree::GetFinalGoalCondition>("GetFinalGoal");
}
