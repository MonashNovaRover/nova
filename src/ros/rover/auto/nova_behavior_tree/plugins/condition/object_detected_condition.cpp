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

// TODO: cluster by id as well as distance
// TODO: highest confidence detections should go to the front of the queue

#include "nova_interfaces/msg/detection2_d_array.hpp"
#include "rclcpp/logging.hpp"

#include "nova_behavior_tree/condition/detected_condition_base.hpp"

namespace nova_behavior_tree
{
  class ObjectDetectedCondition : public DetectedConditionBase<nova_interfaces::msg::Detection2DArray>
  {
  public:
    using Base = DetectedConditionBase<nova_interfaces::msg::Detection2DArray>;

    ObjectDetectedCondition(
      const std::string &condition_name,
      const BT::NodeConfiguration &conf)
      : Base(condition_name, conf) {}

  private:
    void callback(const nova_interfaces::msg::Detection2DArray::SharedPtr msg) override
    {
      for (const auto &detection : msg->detections)
      {
        goal_class_names_.push_back(detection.class_name);
        Goal goal;
        goal.header = msg->header;
        // Normalise y coordinate to be [0, 1] and x coordinate to be [0, aspect_ratio]
        goal.pose.position.x = detection.pose.position.x / detection.image_height;
        goal.pose.position.y = detection.pose.position.y / detection.image_height;
        raw_detections_.push_back(goal);
      }
    }

    void log_detections() override
    {
      for (const auto &class_name : goal_class_names_)
      {
        RCLCPP_INFO(node_->get_logger(), "📍 Object %s detected.", class_name.c_str());
      }
    }

    void clear_processed_detections() override
    {
      Base::clear_processed_detections();
      goal_class_names_.clear();
    }
    
    std::vector<std::string> goal_class_names_;
  };

} // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::ObjectDetectedCondition>("ObjectDetected");
}
