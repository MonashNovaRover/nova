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

#include <string>
#include <memory>
#include <limits>

#include "nav_msgs/msg/path.hpp"
#include "nav2_util/geometry_utils.hpp"
#include "rclcpp/logging.hpp"
#include "tf2/utils.h"

#include "nova_behavior_tree/action/remove_passed_urc_goals_action.hpp"
#include "nova_behavior_tree/nav2_utils.hpp"
#include "nova_behavior_tree/utils.hpp"

namespace nova_behavior_tree
{

  RemovePassedURCGoalsAction::RemovePassedURCGoalsAction(
    const std::string & name,
    const BT::NodeConfiguration & conf)
    : BT::ActionNodeBase(name, conf)
  {
  }

  void RemovePassedURCGoalsAction::initialize()
  {
    tf_ = config().blackboard->get<std::shared_ptr<tf2_ros::Buffer>>("tf_buffer");
    node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
    node_->get_parameter("transform_tolerance", transform_tolerance_);
    robot_base_frame_ = BT::deconflictPortAndParamFrame<std::string>(node_, "robot_base_frame", this);

    getInput("position_tolerance", position_tolerance_);
    getInput("orientation_tolerance", orientation_tolerance_);

    initialized_ = true;
  }

  inline BT::NodeStatus RemovePassedURCGoalsAction::tick()
  {
    // 📝 Initialise the node on startup with static inputs.
    if (!initialized_)
    {
      initialize();
    }

    // 📝 Get new dynamic inputs.
    getInput("input_goals", input_goals_);
    getInput("current_pose", current_pose_);

    // 📝 Remove passed goals
    // ❗ Note we leave one goal for the controller server to remove in FollowPath (i.e. the successful exit condition for the BT).
    using namespace nav2_util::geometry_utils;  // NOLINT
    while (input_goals_.size() > 1) 
    {
      // 📝 Calculate the distance remaining to the next goal.
      double dist_to_goal = euclidean_distance(input_goals_[0].pose, current_pose_.pose);
      setOutput("dist_to_goal", dist_to_goal);

      // 📝 Exit if the distance remaining to the next goal is greater than position_tolerance_.
      if (dist_to_goal > position_tolerance_) 
      {
        break;
      }

      // 📝 Exit if the angle remaining to the next goal is greater than orientation_tolerance_.
      // ❗ Use our custom function instead of angles::shortest_angular_distance
      double rover_yaw = tf2::getYaw(current_pose_.pose.orientation);
      double goal_yaw = tf2::getYaw(input_goals_[0].pose.orientation);
      double angle_to_goal = utils::nav2::shortestAngularDistance(rover_yaw, goal_yaw);
      if (std::fabs(angle_to_goal) > orientation_tolerance_) 
      {
        break;
      }

      // 📝 Remove the next goal
      input_goals_.erase(input_goals_.begin());
      RCLCPP_INFO(
        node_->get_logger(), "Goal reached! Removing goal."
      );
    }

    setOutput("output_goals", input_goals_);

    return BT::NodeStatus::SUCCESS;
  }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::RemovePassedURCGoalsAction>("RemovePassedURCGoals");
}