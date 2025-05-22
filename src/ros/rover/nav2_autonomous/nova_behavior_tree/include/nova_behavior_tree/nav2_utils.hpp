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

#define M_PI 3.14159265358979323846

#include <string>
#include <sstream>

#include "nav2_util/geometry_utils.hpp"
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <geometry_msgs/msg/point.hpp>
#include <geometry_msgs/msg/quaternion.hpp>

namespace nova_behavior_tree::utils::nav2
{

  using namespace geometry_msgs::msg;
  using namespace nav2_util::geometry_utils;

  inline std::string poseStampedToString(const PoseStamped &pose)
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

  inline Quaternion eulerToQuaternion(const double &roll, const double &pitch, const double &yaw)
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

  // Small helper to replicate angles::shortest_angular_distance
  inline double shortestAngularDistance(const double &from, const double &to)
  {
    // Normalizes the difference into (-π, +π]
    double angle = std::fmod(to - from, 2.0 * M_PI);
    if (angle > M_PI)
    {
      angle -= 2.0 * M_PI;
    }
    else if (angle <= -M_PI)
    {
      angle += 2.0 * M_PI;
    }
    return angle;
  }

  /**
   * @brief Orient the pose in 2D towards a target point
   */
  inline void orientTowards(Pose &pose, const Point &target)
  {
    double dx = target.x - pose.position.x;
    double dy = target.y - pose.position.y;
    double yaw = atan2(dy, dx);
    pose.orientation = orientationAroundZAxis(yaw);
  }

  /**
   * @brief Returns an offset goal between the rover and the goal, facing towards the goal
   */
  inline PoseStamped offsetGoal(const PoseStamped &input_goal, const PoseStamped &rover_goal, const double &offset)
  {
    tf2::Vector3 rover;
    tf2::Vector3 goal;
    tf2::fromMsg(input_goal.pose.position, rover);
    tf2::fromMsg(rover_goal.pose.position, goal);
    
    tf2::Vector3 rover_to_goal_normal = (goal - rover).normalized();
    tf2::Vector3 offset_position = goal - rover_to_goal_normal * offset;

    geometry_msgs::msg::PoseStamped offset_goal;
    offset_goal.header = input_goal.header;
    tf2::toMsg(offset_position, offset_goal.pose.position);
    orientTowards(offset_goal.pose, input_goal.pose.position);
    return offset_goal;
  }

  inline double radians(const double &degrees)
  {
    return degrees * M_PI / 180.0;
  }

  inline double degrees(const double &radians)
  {
    return radians * 180.0 / M_PI;
  }

  inline bool arePointsEqual(const Point &p1, const Point &p2)
  {
    return p1.x == p2.x &&
           p1.y == p2.y &&
           p1.z == p2.z;
  }

  inline bool areQuaternionsEqual(const Quaternion &q1, const Quaternion &q2)
  {
    return q1.x == q2.x &&
           q1.y == q2.y &&
           q1.z == q2.z &&
           q1.w == q2.w;
  }

  inline bool isDefaultPose(const Pose &p)
  {
    return arePointsEqual(p.position, Point()) &&
           areQuaternionsEqual(p.orientation, Quaternion());
  }

}

#endif // NOVA_BEHAVIOR_TREE__NAV2_UTILS_HPP_