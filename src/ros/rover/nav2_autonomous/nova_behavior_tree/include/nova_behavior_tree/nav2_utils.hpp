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

#define PI 3.14159265358979323846

#include <string>
#include <sstream>

#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <geometry_msgs/msg/point.hpp>
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

  // Small helper to replicate angles::shortest_angular_distance
  inline double shortestAngularDistance(double from, double to)
  {
    // Normalizes the difference into (-π, +π]
    double angle = std::fmod(to - from, 2.0 * PI);
    if (angle > PI)
    {
      angle -= 2.0 * PI;
    }
    else if (angle <= -PI)
    {
      angle += 2.0 * PI;
    }
    return angle;
  }

  double radians(double degrees)
  {
    return degrees * PI / 180.0;
  }

  double degrees(double radians)
  {
    return radians * 180.0 / PI;
  }
  
  bool isClose(double a, double b, double tolerance = 0.0)
  {
    return abs(a - b) <= tolerance;
  }

  bool arePointsEqual(Point p1, Point p2, double tolerance = 0.0)
  {
    return isClose(p1.x, p2.x, tolerance) && 
           isClose(p1.y, p2.y, tolerance) && 
           isClose(p1.z, p2.z, tolerance);
  }

  bool areQuaternionsEqual(Quaternion q1, Quaternion q2)
  {
    return q1.x == q2.x && q1.y == q2.y && q1.z == q2.z && q1.w == q2.w;
  }

  bool isDefaultPose(Pose p)
  {
    return arePointsEqual(p.position, Point()) &&
           areQuaternionsEqual(p.orientation, Quaternion());
  }

}

#endif // NOVA_BEHAVIOR_TREE__NAV2_UTILS_HPP_