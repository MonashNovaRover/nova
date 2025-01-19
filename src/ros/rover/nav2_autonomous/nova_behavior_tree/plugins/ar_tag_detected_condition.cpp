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

#include <functional>

#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <aruco_opencv_msgs/msg/aruco_detection.hpp>
#include <aruco_opencv_msgs/msg/marker_pose.hpp>
#include <rclcpp/callback_group.hpp>
#include <rclcpp/qos.hpp>
#include <rclcpp/subscription_options.hpp>
#include "rclcpp/logging.hpp"
#include "rclcpp/rclcpp.hpp"

#include "nova_behavior_tree/ar_tag_detected_condition.hpp"

namespace nova_behavior_tree
{

    ARTagDetectedCondition::ARTagDetectedCondition(
        const std::string &condition_name,
        const BT::NodeConfiguration &conf)
        : BT::ConditionNode(condition_name, conf),
          initialized_(false)
    {
    }

    void ARTagDetectedCondition::initialize()
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
            std::bind(&ARTagDetectedCondition::callback_ar_tag, this, std::placeholders::_1), 
            sub_option
        );

        initialized_ = true;
    }

    void ARTagDetectedCondition::callback_ar_tag(const aruco_opencv_msgs::msg::ArucoDetection::SharedPtr msg)
    {
        // Save first ar tag id
        tag_found_ = false;
        for (aruco_opencv_msgs::msg::MarkerPose marker : msg->markers) {
            tag_id_ = marker.marker_id;
            tag_header_ = msg->header;
            tag_pose_.position.x = marker.pose.position.x;
            tag_pose_.position.y = marker.pose.position.y;
            tag_found_ = true;
            return;
        }
    }

    BT::NodeStatus ARTagDetectedCondition::tick()
    {
        if (!initialized_)
        {
            initialize();
        }

        callback_group_executor_.spin_some();

        if (detected())
        {
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::FAILURE;
    }

    bool ARTagDetectedCondition::detected()
    {
        IDs seen_ids;
        getInput("seen_ids", seen_ids);

        if (tag_found_)
        {
            for (int id : seen_ids)
            {
                if (id == tag_id_)
                {
                    return false;
                }
            }
            setOutput("id", tag_id_);
            geometry_msgs::msg::PoseStamped goal;
            goal.header = tag_header_;
            goal.pose = tag_pose_;
            setOutput("goal", goal);
            RCLCPP_INFO(node_->get_logger(), "AR tag found, setting goal");
        }
        
        return tag_found_;
    }


} // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
    factory.registerNodeType<nova_behavior_tree::ARTagDetectedCondition>("ARTagDetected");
}
