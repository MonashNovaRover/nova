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
#include <string>
#include <memory>
#include <visualization_msgs/msg/detail/marker__struct.hpp>
#include <visualization_msgs/msg/detail/marker_array__struct.hpp>

#include "nav2_util/robot_utils.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav2_util/node_utils.hpp"

#include "nova_behavior_tree/query_position_condition.hpp"

namespace nova_behavior_tree
{

    QueryPositionCondition::QueryPositionCondition(
        const std::string &condition_name,
        const BT::NodeConfiguration &conf)
        : BT::ConditionNode(condition_name, conf),
          initialized_(false)
    {
    }

    QueryPositionCondition::~QueryPositionCondition()
    {
        cleanup();
    }

    void QueryPositionCondition::initialize()
    {
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");

        initialized_ = true;

        sub_objects_ = node_->create_subscription<visualization_msgs::msg::MarkerArray>("/oak/nn/spatial_detections_markers", 10, std::bind(&QueryPositionCondition::callback_object_detection, this, _1));

        sub_ar_tags_ = node_->create_subscription<visualization_msgs::msg::MarkerArray>("/oak/nn/spatial_detections_markers", 10, std::bind(&QueryPositionCondition::callback_ar_tag, this, _1));
    }

    void QueryPositionCondition::callback_ar_tag(const visualization_msgs::msg::MarkerArray::SharedPtr msg)
    {
        for (visualization_msgs::msg::Marker marker : msg->markers) {
            const int _id = marker.id;
            tag_markers_[_id] = marker;
        }
    }

    void QueryPositionCondition::callback_object_detection(const visualization_msgs::msg::MarkerArray::SharedPtr msg)
    {
        for (const visualization_msgs::msg::Marker &marker : msg->markers) {
            const int _id = marker.id;
            object_markers_[_id] = marker;
        }
    }

    BT::NodeStatus QueryPositionCondition::tick()
    {
        if (!initialized_)
        {
            initialize();
        }

        if (queryPose())
        {
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::FAILURE;
    }

    bool QueryPositionCondition::queryPose()
    {
        int _id;
        std::string detection_type;
        getInput("id", _id);
        getInput("detection_type", detection_type);
        if (detection_type == "tag") {
            if (tag_markers_.count(_id)) {
                visualization_msgs::msg::Marker chosen_tag = tag_markers_[_id];
                geometry_msgs::msg::PoseStamped pose_stamped;
                pose_stamped.header = chosen_tag.header;
                pose_stamped.pose = chosen_tag.pose;
                
                setOutput("position", pose_stamped);
            } else {
                return false;
            }
        } else if (detection_type == "object") {
            if (object_markers_.count(_id)) {
                visualization_msgs::msg::Marker chosen_object = object_markers_[_id];
                geometry_msgs::msg::PoseStamped pose_stamped;
                pose_stamped.header = chosen_object.header;
                pose_stamped.pose = chosen_object.pose;
                
                setOutput("position", pose_stamped);
            } else {
                return false;
            }
        } else {
            return false;
        }
 
        return true;
    }

} // namespace nova_behavior_tree

#include "behaviortree_cpp_v3/bt_factory.h"
BT_REGISTER_NODES(factory)
{
    factory.registerNodeType<nova_behavior_tree::QueryPositionCondition>("GoalsEmpty");
}
