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

#include <string>
#include <memory>

#include "nav2_util/robot_utils.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav2_util/node_utils.hpp"

#include "nova_behavior_tree/goals_empty_condition.hpp"

namespace nova_behavior_tree
{

    GoalsEmptyCondition::GoalsEmptyCondition(
        const std::string &condition_name,
        const BT::NodeConfiguration &conf)
        : BT::ConditionNode(condition_name, conf),
          initialized_(false)
    {
    }

    GoalsEmptyCondition::~GoalsEmptyCondition()
    {
        cleanup();
    }

    void GoalsEmptyCondition::initialize()
    {
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");

        initialized_ = true;
    }

    BT::NodeStatus GoalsEmptyCondition::tick()
    {
        if (!initialized_)
        {
            initialize();
        }

        if (isGoalsEmpty())
        {
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::FAILURE;
    }

    bool GoalsEmptyCondition::isGoalsEmpty()
    {
        std::vector<geometry_msgs::msg::PoseStamped> goals;
        getInput("goals", goals);
        return goals.size() == 0;
    }

} // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
    factory.registerNodeType<nova_behavior_tree::GoalsEmptyCondition>("GoalsEmpty");
}