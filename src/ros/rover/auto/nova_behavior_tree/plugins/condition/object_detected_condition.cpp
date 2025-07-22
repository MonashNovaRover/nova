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

#include <visualization_msgs/msg/marker_array.hpp>
#include <visualization_msgs/msg/marker.hpp>
#include "rclcpp/logging.hpp"

#include "nova_behavior_tree/condition/detected_condition_base.hpp"

namespace nova_behavior_tree
{
  class ObjectDetectedCondition : public DetectedConditionBase<visualization_msgs::msg::MarkerArray>
  {
  public:
    using Base = DetectedConditionBase<visualization_msgs::msg::MarkerArray>;

    ObjectDetectedCondition(
      const std::string &condition_name,
      const BT::NodeConfiguration &conf)
      : Base(condition_name, conf) {}

  private:
    void callback(const visualization_msgs::msg::MarkerArray::SharedPtr msg) override
    {
      for (const auto &marker : msg->markers)
      {
        if (marker.action == visualization_msgs::msg::Marker::ADD)
        {
          goal_ids_.push_back(marker.id);
          Goal goal;
          goal.header = marker.header;
          goal.pose.position.x = marker.pose.position.x;
          goal.pose.position.y = marker.pose.position.y;
          raw_detections_.push_back(goal);
        }
      }
    }

    void log_detections() override
    {
      for (const auto &id : goal_ids_)
      {
        RCLCPP_INFO(node_->get_logger(), "📍 Object %i found, setting goal.", id);
      }
    }

    void clear_processed_detections() override
    {
      Base::clear_processed_detections();
      goal_ids_.clear();
    }
    
    std::vector<int> goal_ids_;
  };

} // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::ObjectDetectedCondition>("ObjectDetected");
}
