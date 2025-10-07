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

#include <algorithm>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <rclcpp/logging.hpp>

#include "nova_behavior_tree/compute_search_goals_condition.hpp"

namespace nova_behavior_tree
{

    ComputeSearchGoalsCondition::ComputeSearchGoalsCondition(
        const std::string &condition_name,
        const BT::NodeConfiguration &conf)
        : BT::ConditionNode(condition_name, conf),
          initialized_(false)
    {
    }

    ComputeSearchGoalsCondition::~ComputeSearchGoalsCondition()
    {
        cleanup();
    }

    void ComputeSearchGoalsCondition::initialize()
    {
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");

        initialized_ = true;
    }

    BT::NodeStatus ComputeSearchGoalsCondition::tick()
    {
        if (!initialized_)
        {
            initialize();
        }

        if (computeGoals())
        {
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::FAILURE;
    }

    bool ComputeSearchGoalsCondition::computeGoals()
    {
        geometry_msgs::msg::PoseStamped search_origin;
        int radius, n;
        getInput("search_origin", search_origin);
        getInput("search_radius", radius);
        getInput("num_points", n);

        std::vector<geometry_msgs::msg::PoseStamped> search_goals = {search_origin};
        double d_theta = 2 * M_PI / n;

        for (int i = 0; i < n; i++) {
            geometry_msgs::msg::PoseStamped new_point;
            new_point.header = search_origin.header;
            new_point.pose.position.x = search_origin.pose.position.x + radius * std::cos(d_theta * i);
            new_point.pose.position.y = search_origin.pose.position.y + radius * std::sin(d_theta * i);
            search_goals.push_back(new_point);
        }

        setOutput("search_goals", search_goals);

        return true;
    }

} // namespace nova_behavior_tree


#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
    factory.registerNodeType<nova_behavior_tree::ComputeSearchGoalsCondition>("ComputeSearchGoals");
}
