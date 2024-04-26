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
#include <geometry_msgs/msg/detail/pose_stamped__struct.hpp>

#include <aruco_opencv_msgs/msg/detail/aruco_detection__struct.hpp>
#include <aruco_opencv_msgs/msg/detail/marker_pose__struct.hpp>
#include <vision_msgs/msg/detail/detection3_d_array__struct.hpp>

#include "nova_behavior_tree/check_detected_condition.hpp"

namespace nova_behavior_tree
{

    CheckDetectedCondition::CheckDetectedCondition(
        const std::string &condition_name,
        const BT::NodeConfiguration &conf)
        : BT::ConditionNode(condition_name, conf),
          initialized_(false)
    {
    }

    CheckDetectedCondition::~CheckDetectedCondition()
    {
        cleanup();
    }

    void CheckDetectedCondition::initialize()
    {
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");

        initialized_ = true;

        sub_objects_ = node_->create_subscription<vision_msgs::msg::Detection3DArray>("/oak/nn/spatial_detections", 10, std::bind(&CheckDetectedCondition::callback_object_detection, this, _1));

        sub_ar_tags_ = node_->create_subscription<aruco_opencv_msgs::msg::ArucoDetection>("/aruco_detections", 10, std::bind(&CheckDetectedCondition::callback_ar_tag, this, _1));
    }

    void CheckDetectedCondition::callback_ar_tag(const aruco_opencv_msgs::msg::ArucoDetection::SharedPtr msg)
    {
        // Save ar tag id
        for (aruco_opencv_msgs::msg::MarkerPose marker : msg->markers) {
            const int _id = marker.marker_id;
            tags_.insert(_id);
        }
    }

    void CheckDetectedCondition::callback_object_detection(const vision_msgs::msg::Detection3DArray::SharedPtr msg)
    {
        // Save object id
        for (const vision_msgs::msg::Detection3D &detection : msg->detections) {
            const std::string _id = detection.id;
            objects_.insert(_id);
        }
    }

    BT::NodeStatus CheckDetectedCondition::tick()
    {
        if (!initialized_)
        {
            initialize();
        }

        if (detected())
        {
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::FAILURE;
    }

    bool CheckDetectedCondition::detected()
    {
        std::string detection_type;
        getInput("detection_type", detection_type);
        if (detection_type == "tag") {
            std::string str_id;
            getInput("id", str_id);
            int _id = atoi(str_id.c_str());
            return tags_.count(_id) > 0;
        }
        else if (detection_type == "object") {
            std::string _id;
            getInput("id", _id);
            return objects_.count(_id) > 0;
        } else {
            return false;
        }
    }

} // namespace nova_behavior_tree

#include "behaviortree_cpp_v3/bt_factory.h"
BT_REGISTER_NODES(factory)
{
    factory.registerNodeType<nova_behavior_tree::CheckDetectedCondition>("CheckDetected");
}
