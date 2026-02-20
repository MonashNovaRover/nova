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
 * BRIEF: Republish existing transforms between new
 * pairs of frames.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * NODE: transform_republisher
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * PACKAGE:   nova_utils
 * AUTHORS:	  Terry Tian
 * CREATION:  2026
 * EDITED:    2026
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef NOVA_UTILS__TRANSFORM_REPUBLISHER_HPP_
#define NOVA_UTILS__TRANSFORM_REPUBLISHER_HPP_

#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <tuple>
#include <unordered_map>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "tf2_ros/transform_broadcaster.h"

namespace nova_utils
{

class TransformRepublisher : public rclcpp::Node
{
public:
  TransformRepublisher();

private:
  using StringPair = std::pair<std::string, std::string>;
  using StringStringTime = std::tuple<std::string, std::string, rclcpp::Time>;
  void tf_callback(const tf2_msgs::msg::TFMessage::SharedPtr msg);

  std::unordered_map<std::string, StringStringTime> repub_map_;

  rclcpp::Subscription<tf2_msgs::msg::TFMessage>::SharedPtr sub_;
  std::shared_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
};

}  // namespace nova_utils

#endif  // NOVA_UTILS__TRANSFORM_REPUBLISHER_HPP_