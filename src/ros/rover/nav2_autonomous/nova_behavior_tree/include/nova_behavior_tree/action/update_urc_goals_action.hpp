// Copyright (c) 2025 Monash Nova Rover
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

#ifndef NOVA_BEHAVIOR_TREE__PLUGINS__ACTION__UPDATE_GOAL_ACTION_HPP_
#define NOVA_BEHAVIOR_TREE__PLUGINS__ACTION__UPDATE_GOAL_ACTION_HPP_
#include <vector>
#include <memory>
#include <string>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav2_util/geometry_utils.hpp"
#include "behaviortree_cpp/action_node.h"
#include "nav2_behavior_tree/bt_utils.hpp"

namespace nova_behavior_tree
{

class UpdateURCGoalsAction : public BT::ActionNodeBase
{
public:
  typedef geometry_msgs::msg::PoseStamped Goal;
  typedef std::vector<Goal> Goals;

  UpdateURCGoalsAction(
    const std::string & xml_tag_name,
    const BT::NodeConfiguration & conf);

  /**
   * @brief Function to initialize variables,
   * called only once in the lifecycle of the BT
   */
  void initialize();

  static BT::PortsList providedPorts()
  {
    return {
      BT::InputPort<Goal>("detected_goal", "Detected goal (AR tag / object)"),
      BT::InputPort<Goal>("current_pose", "The current pose of the rover"),
      BT::InputPort<double>("offset_radius", "Radius away from actual pose to set goal"),
      BT::InputPort<Goals>("input_goals", "Input goals to be updated"),
      BT::OutputPort<Goals>("output_goals", "Updated goals"),
    };
  }

private:
  void halt() override {}
  BT::NodeStatus tick() override;
  void update_urc_goals();

  rclcpp::Node::SharedPtr node_;

  Goal detected_goal_;
  Goal prev_detected_goal_;
  Goal current_pose_;
  double offset_radius_;
  Goals goals_;

  bool initialized_ = false;
};

}  // namespace nova_behavior_tree

#endif  // NOVA_BEHAVIOR_TREE__PLUGINS__ACTION__UPDATE_GOAL_ACTION_HPP_