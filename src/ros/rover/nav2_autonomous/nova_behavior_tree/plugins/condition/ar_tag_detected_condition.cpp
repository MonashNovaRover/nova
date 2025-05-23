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

#include <aruco_opencv_msgs/msg/aruco_detection.hpp>
#include <aruco_opencv_msgs/msg/marker_pose.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <rclcpp/callback_group.hpp>
#include <rclcpp/qos.hpp>
#include <rclcpp/subscription_options.hpp>
#include "rclcpp/logging.hpp"
#include "rclcpp/rclcpp.hpp"

#include "nova_behavior_tree/condition/ar_tag_detected_condition.hpp"
#include "nova_behavior_tree/nav2_utils.hpp"

namespace nova_behavior_tree
{
  ARTagDetectedCondition::ARTagDetectedCondition(
      const std::string &condition_name,
      const BT::NodeConfiguration &conf) : BT::ConditionNode(condition_name, conf)
  { 
  }

  void ARTagDetectedCondition::initialize()
  {
    node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
    callback_group_ = node_->create_callback_group(
      rclcpp::CallbackGroupType::MutuallyExclusive, 
      false
    );
    callback_group_executor_.add_callback_group(callback_group_, node_->get_node_base_interface());

    rclcpp::SubscriptionOptions sub_option;
    sub_option.callback_group = callback_group_;

    sub_ar_tag_ = node_->create_subscription<aruco_opencv_msgs::msg::ArucoDetection>(
      "/aruco_detections", 
      rclcpp::SystemDefaultsQoS(), 
      std::bind(&ARTagDetectedCondition::callback_ar_tag, this, std::placeholders::_1), 
      sub_option
    );

    getInput("min_detections", min_detections_);
    getInput("buffer_time", buffer_time_);

    initialized_ = true;
  }

  void ARTagDetectedCondition::callback_ar_tag(const aruco_opencv_msgs::msg::ArucoDetection::SharedPtr msg)
  {
    if (!msg->markers.empty())
    {
      aruco_opencv_msgs::msg::MarkerPose marker = msg->markers[0];
      goal_id_ = marker.marker_id;
      goal_.header= msg->header;
      goal_.pose.position.x = marker.pose.position.x;
      goal_.pose.position.y = marker.pose.position.y;
      goal_found_ = true;
      // RCLCPP_INFO(node_->get_logger(), "Detected markers: %d\n%s", msg->markers.size(), utils::nav2::poseStampedToString(goal_).c_str());
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
    if (goal_found_)
    {
      // detection filtering
      while (!detections_buffer_.empty() && (node_->now() - detections_buffer_.front().header.stamp).seconds() > buffer_time_)
      {
          detections_buffer_.pop();
      }
      detections_buffer_.push(goal_);

      if (detections_buffer_.size() >= min_detections_)
      {
        // detection is valid
        setOutput("goal", goal_);
        goal_found_ = false;
        RCLCPP_INFO(node_->get_logger(), "📍 AR tag %i found, setting goal.", goal_id_);
        return true;
      }
    }
    return false;
  }

} // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::ARTagDetectedCondition>("ARTagDetected");
}
