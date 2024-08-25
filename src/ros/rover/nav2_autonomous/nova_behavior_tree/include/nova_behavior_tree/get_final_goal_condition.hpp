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

#ifndef NAV2_BEHAVIOR_TREE__PLUGINS__CONDITION__GET_FINAL_GOAL_CONDITION_HPP_
#define NAV2_BEHAVIOR_TREE__PLUGINS__CONDITION__GET_FINAL_GOAL_CONDITION_HPP_

#include <behaviortree_cpp/basic_types.h>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <vector>
#include <string>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "behaviortree_cpp/condition_node.h"
#include "tf2_ros/buffer.h"

namespace nova_behavior_tree
{

    /**
     * @brief A BT::ConditionNode that returns SUCCESS when the first goal in a vector of poses
     * is reached and FAILURE otherwise
     */
    class GetFinalGoalCondition : public BT::ConditionNode
    {
    public:
        /**
         * @brief A constructor for nova_behavior_tree::FirstGoalReachedCondition
         * @param condition_name Name for the XML tag for this node
         * @param conf BT node configuration
         */
        GetFinalGoalCondition(
            const std::string &condition_name,
            const BT::NodeConfiguration &conf);

        GetFinalGoalCondition() = delete;

        /**
         * @brief A destructor for nav2_behavior_tree::FirstGoalReachedCondition
         */
        ~GetFinalGoalCondition() override;

        /**
         * @brief The main override required by a BT action
         * @return BT::NodeStatus Status of tick execution
         */
        BT::NodeStatus tick() override;

        /**
         * @brief Function to read parameters and initialize class variables
         */
        void initialize();

        /**
         * @brief Checks if the current robot pose lies within a given distance from the first goal
         * @return bool true when goal is reached, false otherwise
         */
        bool getFinalGoal();

        /**
         * @brief Creates list of BT ports
         * @return BT::PortsList Containing node-specific ports
         */
        static BT::PortsList providedPorts()
        {
            return {
                BT::InputPort<std::vector<geometry_msgs::msg::PoseStamped>>("goals", "Waypoints"),
                BT::OutputPort<geometry_msgs::msg::PoseStamped>("final_goal", "Last PoseStamped in the list")};
        }

    protected:
        /**
         * @brief Cleanup function
         */
        void cleanup()
        {
        }

    private:
        rclcpp::Node::SharedPtr node_;

        bool initialized_;
    };

} // namespace nova_behavior_tree

#endif // NAV2_BEHAVIOR_TREE__PLUGINS__CONDITION__GET_FINAL_GOAL_CONDITION_HPP_
