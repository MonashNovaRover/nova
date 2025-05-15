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

/**
 * @brief Condition node for checking whether two poses are nearby. If the input
 * poses are in different frames, it will automatically transform both to the global frame.
 * 
 * Ported over from Nav2 main branch.
 * 
 * @authors Terry Tian
 */

#ifndef NAV2_BEHAVIOR_TREE__PLUGINS__CONDITION__ARE_POSES_NEAR_CONDITION_HPP_
#define NAV2_BEHAVIOR_TREE__PLUGINS__CONDITION__ARE_POSES_NEAR_CONDITION_HPP_

#include <string>
#include <memory>

#include "behaviortree_cpp/condition_node.h"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "rclcpp/rclcpp.hpp"
#include "tf2_ros/buffer.h"

namespace nova_behavior_tree
{

  class ArePosesNearCondition : public BT::ConditionNode
  {
  public:
    /**
     * @brief A constructor for nova_behavior_tree::ARTagDetectedCondition
     * @param condition_name Name for the XML tag for this node
     * @param conf BT node configuration
     */
    ArePosesNearCondition(
        const std::string &condition_name,
        const BT::NodeConfiguration &conf);

    ArePosesNearCondition() = delete;

    /**
     * @brief Function to initialize variables,
     * called only once in the lifecycle of the BT
     */
    void initialize();

    /**
     * @brief The main override required by a BT node
     * @return BT::NodeStatus Status of tick execution
     */
    BT::NodeStatus tick() override;

    /**
     * @brief Creates list of BT ports
     * @return BT::PortsList Containing node-specific ports
     */
    static BT::PortsList providedPorts()
    {
      return {
        BT::InputPort<geometry_msgs::msg::PoseStamped>("from_pose", "From pose"),
        BT::InputPort<geometry_msgs::msg::PoseStamped>("to_pose", "Second pose"),
        BT::InputPort<std::string>("global_frame", "Global frame"),
        BT::InputPort<double>("tolerance", "Tolerance"),
      };
    }

  private:
    /**
     * @brief Checks if the current robot pose lies within a given distance from the goal
     * @return bool true when goal is reached, false otherwise
     */
    bool are_poses_nearby();

    rclcpp::Node::SharedPtr node_;
    std::shared_ptr<tf2_ros::Buffer> tf_;

    double transform_tolerance_;
    std::string global_frame_;
    
    bool initialized_ = false;
  };

} // namespace nova_behavior_tree

#endif // NAV2_BEHAVIOR_TREE__PLUGINS__CONDITION__ARE_POSES_NEAR_CONDITION_HPP_