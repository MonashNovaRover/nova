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
 * @brief Action node for updating/inserting goals. Goals are inserted based
 * on their distance to the rover, with the closest goals at the front and
 * furthest at the back.
 * 
 * @authors Terry Tian
 */

#ifndef NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__UPDATE_GOALS_ACTION_HPP_
#define NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__UPDATE_GOALS_ACTION_HPP_

#include <vector>
#include <memory>
#include <string>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "nav2_util/geometry_utils.hpp"
#include "behaviortree_cpp/action_node.h"
#include "nav2_behavior_tree/bt_utils.hpp"

namespace nova_behavior_tree
{

class UpdateGoalsAction : public BT::ActionNodeBase
{
public:
  typedef geometry_msgs::msg::PoseStamped Goal;
  typedef std::vector<Goal> Goals;

  struct GoalEntry {
    geometry_msgs::msg::Pose pose{};
    size_t index = 0;
  };

  UpdateGoalsAction(
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
      BT::InputPort<std::string>("goal_type", "Type of goal being updated (for logging)"),
      BT::InputPort<Goal>("current_pose", "The current pose of the rover"),
      BT::InputPort<Goals>("goals", "Goals to add as viapoints"),
      BT::InputPort<Goals>("input_goals", "Original goals to add viapoints into"),
      BT::InputPort<double>("update_radius", 0.5, "Radius to next goal for it to be considered an update"),
      BT::InputPort<double>("goal_radius", 0.5, "Radius away from actual pose to set goal"),
      BT::OutputPort<Goals>("output_goals", "Goals with new viapoints added/updated"),
    };
  }

private:
  void halt() override {}
  BT::NodeStatus tick() override;
  void update_goals();

  rclcpp::Node::SharedPtr node_;

  double viapoint_overwrite_tolerance_;
  double goal_radius_;
  std::string goal_type_;
  Goal current_pose_;
  Goals goals_;
  Goals input_goals_;
  std::vector<GoalEntry> prev_goals_;
  
  bool initialized_ = false;
};

}  // namespace nova_behavior_tree

#endif  // NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__UPDATE_GOALS_ACTION_HPP_