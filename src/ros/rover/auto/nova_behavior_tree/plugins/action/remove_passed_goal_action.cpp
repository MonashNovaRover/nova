// Copyright (c) 2026 Monash Nova Rover
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

#include <string>
#include <memory>
#include <limits>

#include "nav_msgs/msg/path.hpp"
#include "nav2_util/geometry_utils.hpp"

#include "nova_behavior_tree/action/remove_passed_goal_action.hpp"
#include "nova_behavior_tree/nav2_utils.hpp"
#include "nova_behavior_tree/utils.hpp"

namespace nova_behavior_tree
{

  RemovePassedGoalAction::RemovePassedGoalAction(
    const std::string & name,
    const BT::NodeConfiguration & conf)
  : BT::ActionNodeBase(name, conf),
    viapoint_achieved_radius_(0.5)
  {}

  void RemovePassedGoalAction::initialize()
  {
    getInput("radius", viapoint_achieved_radius_);

    tf_ = config().blackboard->get<std::shared_ptr<tf2_ros::Buffer>>("tf_buffer");
    auto node = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
    node->get_parameter("transform_tolerance", transform_tolerance_);

    robot_base_frame_ = BT::deconflictPortAndParamFrame<std::string>(
      node, "robot_base_frame", this);
  }

  inline BT::NodeStatus RemovePassedGoalAction::tick()
  {
    if (!BT::isStatusActive(status())) {
      initialize();
    }

    Goals goal_poses;
    getInput("input_goals", goal_poses);

    if (goal_poses.empty()) {
      setOutput("output_goals", goal_poses);
      return BT::NodeStatus::SUCCESS;
    }

    using namespace nav2_util::geometry_utils;  // NOLINT

    geometry_msgs::msg::PoseStamped current_pose;
    if (!nav2_util::getCurrentPose(
        current_pose, *tf_, goal_poses[0].header.frame_id, robot_base_frame_,
        transform_tolerance_))
    {
      return BT::NodeStatus::FAILURE;
    }

    double dist_to_goal;
    dist_to_goal = euclidean_distance(goal_poses[0].pose, current_pose.pose);
    if (dist_to_goal <= viapoint_achieved_radius_) {
      goal_poses.erase(goal_poses.begin());
      setOutput("output_goals", goal_poses);
    }

    return BT::NodeStatus::SUCCESS;
  }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::RemovePassedGoalAction>("RemovePassedGoal");
}