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

#ifndef NAV2_BEHAVIOR_TREE__PLUGINS__CONDITION__AR_TAG_DETECTED_CONDITION_HPP_
#define NAV2_BEHAVIOR_TREE__PLUGINS__CONDITION__AR_TAG_DETECTED_CONDITION_HPP_

#include <aruco_opencv_msgs/msg/aruco_detection.hpp>
#include <behaviortree_cpp/basic_types.h>
#include <functional>
#include <rclcpp/callback_group.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp/subscription.hpp>
#include <string>
#include <cstdlib>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/pose.hpp>

#include <rclcpp/rclcpp.hpp>
#include <vector>

#include "behaviortree_cpp/condition_node.h"


namespace nova_behavior_tree
{

    /**
     * @brief A BT::ConditionNode that returns SUCCESS when an AR tag has been detected and FAILURE otherwise
     */
    class ARTagDetectedCondition : public BT::ConditionNode
    {
    public:
        typedef std::vector<int> IDs;

        /**
         * @brief A constructor for nova_behavior_tree::ARTagDetectedCondition
         * @param condition_name Name for the XML tag for this node
         * @param conf BT node configuration
         */
        ARTagDetectedCondition(
            const std::string &condition_name,
            const BT::NodeConfiguration &conf);

        ARTagDetectedCondition() = delete;

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
         * @brief Creates list of BT ports
         * @return BT::PortsList Containing node-specific ports
         */
        static BT::PortsList providedPorts()
        {
            return {
                BT::InputPort<IDs>("seen_ids", "IDs of visited AR tags"),
                BT::OutputPort<int>("id", "ID of detected AR tag"),
                BT::OutputPort<geometry_msgs::msg::PoseStamped>("goal", "Pose of detected AR tag"),
            };
        }

    private:
        /**
         * @brief Callback to handle AR tag detections
         */
        void callback_ar_tag(const aruco_opencv_msgs::msg::ArucoDetection::SharedPtr msg);

        /**
         * @brief Looks for a detected tag
         * @return bool true when the tag exists, else false
         */
        bool detected();

        rclcpp::Node::SharedPtr node_;

        bool initialized_;

        int tag_id_;
        std_msgs::msg::Header tag_header_;
        geometry_msgs::msg::Pose tag_pose_;
        bool tag_found_;

        rclcpp::CallbackGroup::SharedPtr callback_group_;
        rclcpp::executors::MultiThreadedExecutor callback_group_executor_;
        rclcpp::Subscription<aruco_opencv_msgs::msg::ArucoDetection>::SharedPtr sub_ar_tag_;
    };

} // namespace nova_behavior_tree

#endif // NAV2_BEHAVIOR_TREE__PLUGINS__CONDITION__AR_TAG_DETECTED_CONDITION_HPP_
