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

#ifndef NAV2_BEHAVIOR_TREE__PLUGINS__CONDITION__QUERY_POSITION_CONDITION_HPP_
#define NAV2_BEHAVIOR_TREE__PLUGINS__CONDITION__QUERY_POSITION_CONDITION_HPP_

#include <aruco_opencv_msgs/msg/aruco_detection.hpp>
#include <behaviortree_cpp/basic_types.h>
#include <rclcpp/callback_group.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp/subscription.hpp>
#include <string>
#include <cstdlib>
#include <map>

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <vision_msgs/msg/detection3_d_array.hpp>
#include <vision_msgs/msg/detail/detection3_d__struct.hpp>

#include "behaviortree_cpp/condition_node.h"

using std::placeholders::_1;


namespace nova_behavior_tree
{

    /**
     * @brief A BT::ConditionNode that returns SUCCESS when the first goal in a vector of poses
     * is reached and FAILURE otherwise
     */
    class QueryPositionCondition : public BT::ConditionNode
    {
    public:
        /**
         * @brief A constructor for nova_behavior_tree::GoalsEmptyCondition
         * @param condition_name Name for the XML tag for this node
         * @param conf BT node configuration
         */
        QueryPositionCondition(
            const std::string &condition_name,
            const BT::NodeConfiguration &conf);

        QueryPositionCondition() = delete;

        /**
         * @brief A destructor for nav2_behavior_tree::GoalsEmptyCondition
         */
        ~QueryPositionCondition() override;

        /**
         * @brief The main override required by a BT action
         * @return BT::NodeStatus Status of tick execution
         */
        BT::NodeStatus tick() override;

        /**
         * @brief Function to read parameters and initialize class variables
         */
        void initialize();

        /**
         * @brief Callback to handle AR tag detections
         */
        void callback_ar_tag(const aruco_opencv_msgs::msg::ArucoDetection::SharedPtr msg);

        /**
         * @brief Callback to handle object detections
         */
        void callback_object_detection(const vision_msgs::msg::Detection3DArray::SharedPtr msg);
    
        /**
         * @brief Looks for a detected object/tag in the relevant map and sets the output port to its position
         * @return bool true when the object/tag exists in the map, else false
         */
        bool queryPose();

        /**
         * @brief Creates list of BT ports
         * @return BT::PortsList Containing node-specific ports
         */
        static BT::PortsList providedPorts()
        {
            return {
                BT::InputPort<std::string>("id", "ID of detected object/tag"),
                BT::InputPort<std::string>("detection_type", "Type of detection (object detection or AR tag)"),
                BT::OutputPort<geometry_msgs::msg::PoseStamped>("position", "PoseStamped"),
            };
        }

    protected:
        /**
         * @brief Cleanup function
         */
        void cleanup()
        {
        }

    private:
        rclcpp::Node::SharedPtr node_;
        

        bool initialized_;

        std::map<int, geometry_msgs::msg::PoseStamped> tag_poses_;
        std::map<std::string, geometry_msgs::msg::PoseStamped> object_poses_;

        rclcpp::CallbackGroup::SharedPtr callback_group_;
        rclcpp::executors::MultiThreadedExecutor callback_group_executor_;
        rclcpp::Subscription<aruco_opencv_msgs::msg::ArucoDetection>::SharedPtr sub_ar_tags_;
        rclcpp::Subscription<vision_msgs::msg::Detection3DArray>::SharedPtr sub_objects_;
    };

} // namespace nova_behavior_tree

#endif // NAV2_BEHAVIOR_TREE__PLUGINS__CONDITION__QUERY_POSITION_CONDITION_HPP_
