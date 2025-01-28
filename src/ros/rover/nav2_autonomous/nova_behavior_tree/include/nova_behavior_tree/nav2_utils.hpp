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

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "rclcpp/logging.hpp"

namespace nova_behavior_tree
{

    std::string poseStampedToString(const geometry_msgs::msg::PoseStamped &pose)
    {
        std::ostringstream oss;
        oss << "\nPoseStamped :\n"
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

}

#endif // NOVA_BEHAVIOR_TREE__NAV2_UTILS_HPP_