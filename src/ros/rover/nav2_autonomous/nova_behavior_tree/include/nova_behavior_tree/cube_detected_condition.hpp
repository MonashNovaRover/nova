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
 * @brief Condition node for checking for detected cubes. Detected cubes' poses
 * are added as viapoints in UpdateGoals, and saved to a list for future reference.
 * 
 * @authors Terry Tian
 */

#ifndef NAV2_BEHAVIOR_TREE__PLUGINS__CONDITION__CUBE_DETECTED_CONDITION_HPP_
#define NAV2_BEHAVIOR_TREE__PLUGINS__CONDITION__CUBE_DETECTED_CONDITION_HPP_

#include <vector>
#include <memory>
#include <array>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include "rclcpp/rclcpp.hpp"
#include "behaviortree_cpp/condition_node.h"

namespace nova_behavior_tree
{

  class CubeDetectedCondition : public BT::ConditionNode
  {
  public:
    typedef std::array<bool, 4> IDs;
    typedef std::array<std::vector<geometry_msgs::msg::Pose>, 4> CubePoses;
    typedef geometry_msgs::msg::PoseStamped Goal;
    typedef std::vector<Goal> Goals;

    /**
     * @brief A constructor for nova_behavior_tree::ARTagDetectedCondition
     * @param condition_name Name for the XML tag for this node
     * @param conf BT node configuration
     */
    CubeDetectedCondition(
        const std::string &condition_name,
        const BT::NodeConfiguration &conf);

    CubeDetectedCondition() = delete;

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
          BT::InputPort<IDs>("visited_ids", "visited_ids[i] = true if visited, false otherwise"),
          BT::OutputPort<Goal>("current_pose", "The current pose of the rover"),
          BT::OutputPort<Goals>("cube_goals", "Goals of detected cubes"),
          BT::OutputPort<std::shared_ptr<CubePoses>>("cube_poses", "List of cube poses"),
      };
    }

  private:
    /**
     * @brief Looks for detected cube(s)
     * @return bool true when a cube is detected, else false
     */
    bool detected();

    rclcpp::Node::SharedPtr node_;
    std::shared_ptr<tf2_ros::Buffer> tf_;
    std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
    std::shared_ptr<CubePoses> cube_poses_;

    double transform_tolerance_;
    std::string global_frame_;
    std::string robot_base_frame_;
    
    bool initialized_ = false;
    const std::string COLORS[4] = {"red", "green", "blue", "white"};
  };

} // namespace nova_behavior_tree

#endif // NAV2_BEHAVIOR_TREE__PLUGINS__CONDITION__CUBE_DETECTED_CONDITION_HPP_