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

#ifndef NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__COMPUTE_AR_TAG_GOALS_ACTION_HPP_
#define NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__COMPUTE_AR_TAG_GOALS_ACTION_HPP_

#include <vector>
#include <memory>
#include <string>

#include <aruco_opencv_msgs/msg/aruco_detection.hpp>
#include <behaviortree_cpp/basic_types.h>
#include <functional>
#include <rclcpp/callback_group.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp/subscription.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/header.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/pose_array.hpp>
#include "nav2_util/robot_utils.hpp"
#include "behaviortree_cpp/action_node.h"
#include "nav2_behavior_tree/bt_utils.hpp"

namespace nova_behavior_tree
{

    class ComputeARTagGoals : public BT::ActionNodeBase
    {
    public:
        typedef std::vector<geometry_msgs::msg::PoseStamped> Goals;

        ComputeARTagGoals(
            const std::string & xml_tag_name,
            const BT::NodeConfiguration & conf);

        /**
         * @brief Function to read parameters and initialize class variables
         */
        void initialize();

        static BT::PortsList providedPorts()
        {
            return {
            BT::OutputPort<Goals>("output_goals", "AR tag goals"),
            };
        }

    private:
        void halt() override {}
        BT::NodeStatus tick() override;

        /**
         * @brief Callback to handle AR tag detections
         */
        void callback_ar_tag(const aruco_opencv_msgs::msg::ArucoDetection::SharedPtr msg);

        rclcpp::Node::SharedPtr node_;

        bool initialized_;

        bool tag_found_;
        std_msgs::msg::Header tag_header_;
        geometry_msgs::msg::Pose tag_pose_;

        rclcpp::CallbackGroup::SharedPtr callback_group_;
        rclcpp::executors::MultiThreadedExecutor callback_group_executor_;
        rclcpp::Subscription<aruco_opencv_msgs::msg::ArucoDetection>::SharedPtr sub_ar_tag_;
    };

}  // namespace nova_behavior_tree

#endif  // NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__COMPUTE_AR_TAG_GOALS_ACTION_HPP_