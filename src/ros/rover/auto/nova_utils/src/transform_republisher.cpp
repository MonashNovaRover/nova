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

/**
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Monash Nova Rover Team
 *
 * PACKAGE:   nova_utils
 * AUTHORS:	  Terry Tian
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <memory>
#include <functional>
#include <vector>
#include <string>

#include "geometry_msgs/msg/transform_stamped.hpp"

#include "nova_utils/transform_republisher.hpp"

namespace nova_utils
{

TransformRepublisher::TransformRepublisher()
  : Node("transform_republisher")
  , tf_broadcaster_(std::make_unique<tf2_ros::TransformBroadcaster>(this))
{
  this->declare_parameter("source_parents", std::vector<std::string>());
  this->declare_parameter("source_children", std::vector<std::string>());
  this->declare_parameter("repub_parents", std::vector<std::string>());
  this->declare_parameter("repub_children", std::vector<std::string>());

  auto source_parents = this->get_parameter("source_parents").as_string_array();
  auto source_children = this->get_parameter("source_children").as_string_array();
  auto repub_parents = this->get_parameter("repub_parents").as_string_array();
  auto repub_children = this->get_parameter("repub_children").as_string_array();

  size_t n = source_parents.size();
  if (source_children.size() != n || repub_parents.size() != n || repub_children.size() != n)
  {
    throw std::runtime_error("Transform parameter arrays must be equal length");
  }

  // Construct repub_map_
  for (size_t i = 0; i < n; ++i)
  {
    auto key = source_parents[i] + " -> " + source_children[i];
    auto value = std::make_tuple(repub_parents[i], repub_children[i], rclcpp::Time(0));
    repub_map_[key] = value;
  }

  sub_ = this->create_subscription<tf2_msgs::msg::TFMessage>(
    "/tf", rclcpp::SensorDataQoS(),
    [this](tf2_msgs::msg::TFMessage::SharedPtr msg)
    {
      tf_callback(msg);
    });
}

void TransformRepublisher::tf_callback(const tf2_msgs::msg::TFMessage::SharedPtr msg)
{
  std::vector<geometry_msgs::msg::TransformStamped> repub_tfs;
  for (const geometry_msgs::msg::TransformStamped& t : msg->transforms)
  {
    // if (t.header.frame_id == "map" && t.child_frame_id == "base_link")
    auto key = t.header.frame_id + " -> " + t.child_frame_id;
    if (repub_map_.count(key))
    {
      auto [repub_parent_frame, repub_child_frame, prev_stamp] = repub_map_[key];
      if (rclcpp::Time(t.header.stamp) > prev_stamp)
      {
        auto repub_tf = t;
        repub_tf.header.frame_id = repub_parent_frame;
        repub_tf.child_frame_id = repub_child_frame;
        std::get<2>(repub_map_[key]) = repub_tf.header.stamp;

        repub_tfs.push_back(repub_tf);
      }
    }
  }

  tf_broadcaster_->sendTransform(repub_tfs);
}

}  // namespace nova_utils

int main(int argc, char* argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<nova_utils::TransformRepublisher>());
  rclcpp::shutdown();
  return 0;
}