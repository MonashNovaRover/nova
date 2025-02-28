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

#ifndef NOVA_BEHAVIOR_TREE__NAV2_UTILS_HPP
#define NOVA_BEHAVIOR_TREE__NAV2_UTILS_HPP

#include <string>
#include <sstream>

#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <geometry_msgs/msg/quaternion.hpp>
#include <rclcpp/logging.hpp>

namespace nova_behavior_tree::utils::nav2
{

  using namespace geometry_msgs::msg;

  std::string poseStampedToString(const PoseStamped &pose)
  {
    std::ostringstream oss;
    oss << "\nPoseStamped:\n"
        << "  Header:\n"
        << "    frame_id: " << pose.header.frame_id.c_str() << "\n"
        << "    stamp: " << pose.header.stamp.sec << "." << pose.header.stamp.nanosec << "\n"
        << "  Pose:\n"
        << "    position: " << "[x: " << pose.pose.position.x
        << ", y: " << pose.pose.position.y
        << ", z: " << pose.pose.position.z << "]\n"
        << "    orientation: " << "[x: " << pose.pose.orientation.x
        << ", y: " << pose.pose.orientation.y
        << ", z: " << pose.pose.orientation.z
        << ", w: " << pose.pose.orientation.w << "]\n";

    return oss.str();
  }

  Quaternion eulerToQuaternion(double roll, double pitch, double yaw)
  {
    double cr = cos(roll * 0.5);
    double sr = sin(roll * 0.5);
    double cp = cos(pitch * 0.5);
    double sp = sin(pitch * 0.5);
    double cy = cos(yaw * 0.5);
    double sy = sin(yaw * 0.5);

    Quaternion q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;

    return q;
  }

  PoseStamped poseStampedFromGzPose(
      const std::string &frame_id, rclcpp::Node::SharedPtr node, // to get current time
      const double x, const double y, const double z,
      const double roll, const double pitch, const double yaw)
  {
    PoseStamped pose;
    pose.header.frame_id = frame_id;
    pose.header.stamp = node->now();
    pose.pose.position.x = x;
    pose.pose.position.y = y;
    pose.pose.position.z = z;
    pose.pose.orientation = eulerToQuaternion(roll, pitch, yaw);

    return pose;
  }
  
}

#endif // NOVA_BEHAVIOR_TREE__NAV2_UTILS_HPP_