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

#include <geometry_msgs/msg/pose_stamped.hpp>
#include <aruco_opencv_msgs/msg/aruco_detection.hpp>
#include <aruco_opencv_msgs/msg/marker_pose.hpp>
#include <rclcpp/callback_group.hpp>
#include <rclcpp/qos.hpp>
#include <rclcpp/subscription_options.hpp>

#include "nova_behavior_tree/compute_ar_tag_goals_action.hpp"

namespace nova_behavior_tree
{

    ComputeARTagGoals::ComputeARTagGoals(
    const std::string & name,
    const BT::NodeConfiguration & conf)
    : BT::ActionNodeBase(name, conf),
        initialized_(false)
    {}

    void ComputeARTagGoals::initialize()
    {
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
        callback_group_ = node_->create_callback_group(
            rclcpp::CallbackGroupType::MutuallyExclusive, 
            false);
        callback_group_executor_.add_callback_group(callback_group_, node_->get_node_base_interface());

        rclcpp::SubscriptionOptions sub_option;
        sub_option.callback_group = callback_group_;

        sub_ar_tag_ = node_->create_subscription<aruco_opencv_msgs::msg::ArucoDetection>(
            "/aruco_detections", 
            rclcpp::SystemDefaultsQoS(), 
            std::bind(&ComputeARTagGoals::callback_ar_tag, this, std::placeholders::_1), 
            sub_option
        );

        initialized_ = true;
    }

    void ComputeARTagGoals::callback_ar_tag(const aruco_opencv_msgs::msg::ArucoDetection::SharedPtr msg)
    {
        // Save first ar tag pose
        tag_found_ = false;
        tag_header_ = msg->header;
        for (aruco_opencv_msgs::msg::MarkerPose marker : msg->markers) {
            tag_pose_ = marker.pose;
            tag_found_ = true;
            return;
        }
    }

    inline BT::NodeStatus ComputeARTagGoals::tick()
    {
        if (!BT::isStatusActive(status())) {
            initialize();
        }

        if (tag_found_) {
            Goals goal_poses;
            geometry_msgs::msg::PoseStamped goal_pose;

            goal_pose.pose = tag_pose_;
            goal_pose.header = tag_header_;

            goal_poses.push_back(goal_pose);

            setOutput("output_goals", goal_poses);

            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::RUNNING;
    }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
factory.registerNodeType<nova_behavior_tree::ComputeARTagGoals>("ComputeARTagGoals");
}