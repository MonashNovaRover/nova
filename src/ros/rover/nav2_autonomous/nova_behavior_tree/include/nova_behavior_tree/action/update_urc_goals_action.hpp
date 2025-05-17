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
  typedef std::vector<geometry_msgs::msg::PoseStamped> Goals;

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
      BT::InputPort<geometry_msgs::msg::PoseStamped>("new_goal", "New pose to update goal with"),
      BT::InputPort<Goals>("input_goals", "Goals"),
      BT::InputPort<double>("max_update_radius", 0.5, "Max distance of new pose to current goal to be a valid update"),
      BT::OutputPort<Goals>("output_goals", "Goals"),
    };
  }

private:
  void halt() override {}
  BT::NodeStatus tick() override;

  rclcpp::Node::SharedPtr node_;

  double max_update_radius_ = 0.5;
  bool initialized_ = false;
};

}  // namespace nova_behavior_tree

#endif  // NOVA_BEHAVIOR_TREE__PLUGINS__ACTION__UPDATE_GOAL_ACTION_HPP_