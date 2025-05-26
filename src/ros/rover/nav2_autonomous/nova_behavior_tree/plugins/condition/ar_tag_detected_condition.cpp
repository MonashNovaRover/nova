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

#include "aruco_opencv_msgs/msg/aruco_detection.hpp"
#include "aruco_opencv_msgs/msg/marker_pose.hpp"
#include "rclcpp/logging.hpp"

#include "nova_behavior_tree/condition/detected_condition_base.hpp"

namespace nova_behavior_tree
{

  class ARTagDetectedCondition : public DetectedConditionBase<aruco_opencv_msgs::msg::ArucoDetection>
  {
  public:
    using Base = DetectedConditionBase<aruco_opencv_msgs::msg::ArucoDetection>;

    ARTagDetectedCondition(
      const std::string &condition_name,
      const BT::NodeConfiguration &conf)
      : Base(condition_name, conf) {}

  private:
    void callback(const aruco_opencv_msgs::msg::ArucoDetection::SharedPtr msg) override
    {
      for (const auto &marker : msg->markers)
      {
        goal_ids_.push_back(marker.marker_id);
        Goal goal;
        goal.header = msg->header;
        goal.pose.position.x = marker.pose.position.x;
        goal.pose.position.y = marker.pose.position.y;
        raw_detections_.push_back(goal);
      }
    }

    void clear_processed_detections() override
    {
      Base::clear_processed_detections();
      goal_ids_.clear();
    }

    void log_detections() override
    {
      for (const auto &id : goal_ids_)
      {
        RCLCPP_INFO(node_->get_logger(), "📍 AR tag %i found, setting goal.", id);
      }
    }

    std::vector<int> goal_ids_;
  };

} // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::ARTagDetectedCondition>("ARTagDetected");
}
