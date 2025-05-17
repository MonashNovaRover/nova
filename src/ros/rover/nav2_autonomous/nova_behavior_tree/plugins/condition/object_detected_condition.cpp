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
#include <visualization_msgs/msg/marker_array.hpp>
#include <visualization_msgs/msg/marker.hpp>
#include <rclcpp/callback_group.hpp>
#include <rclcpp/qos.hpp>
#include <rclcpp/subscription_options.hpp>
#include "rclcpp/logging.hpp"
#include "rclcpp/rclcpp.hpp"
#include "nova_behavior_tree/condition/object_detected_condition.hpp"

namespace nova_behavior_tree
{
  ObjectDetectedCondition::ObjectDetectedCondition(
      const std::string &condition_name,
      const BT::NodeConfiguration &conf) : BT::ConditionNode(condition_name, conf)
  { 
  }

  void ObjectDetectedCondition::initialize()
  {
    node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
    callback_group_ = node_->create_callback_group(
      rclcpp::CallbackGroupType::MutuallyExclusive, 
      false
    );
    callback_group_executor_.add_callback_group(callback_group_, node_->get_node_base_interface());

    rclcpp::SubscriptionOptions sub_option;
    sub_option.callback_group = callback_group_;

    sub_object_ = node_->create_subscription<visualization_msgs::msg::MarkerArray>(
      "/yolo/objects", 
      rclcpp::SystemDefaultsQoS(), 
      std::bind(&ObjectDetectedCondition::callback_object, this, std::placeholders::_1), 
      sub_option
    );

    initialized_ = true;
  }

  void ObjectDetectedCondition::callback_object(const visualization_msgs::msg::MarkerArray::SharedPtr msg)
  {
    goal_found_ = 0;

    for (visualization_msgs::msg::Marker marker : msg->markers) {
      goal_id_ = marker.id;
      goal_header_ = marker.header;
      goal_pose_.position.x = marker.pose.position.x;
      goal_pose_.position.y = marker.pose.position.y;
      goal_found_ = 1;
      return;
    }
  }

  BT::NodeStatus ObjectDetectedCondition::tick()
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

  bool ObjectDetectedCondition::detected()
  {
    IDs seen_ids;
    getInput("seen_ids", seen_ids);

    if (goal_found_)
    {
      for (int id : seen_ids)
      {
        if (id == goal_id_)
        {
          return false;
        }
      }
      
      setOutput("id", goal_id_);
      setOutput("found", goal_found_);

      geometry_msgs::msg::PoseStamped goal;
      goal.header = goal_header_;
      goal.pose = goal_pose_;
      setOutput("goal", goal);

      RCLCPP_INFO(node_->get_logger(), "📍 Object %i found, setting goal.", goal_id_);
    }
    return goal_found_;
  }

} // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::ObjectDetectedCondition>("ObjectDetected");
}
