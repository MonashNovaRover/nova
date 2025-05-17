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
    Goals goals;
    getInput("input_goals", goals);

    if (goals.empty()) 
    {
      return BT::NodeStatus::SUCCESS;
    }

    // 📝 Calculate the distance remaining to the next goal.
    using namespace nav2_util::geometry_utils;  // NOLINT

    geometry_msgs::msg::PoseStamped rover_pose;
    if (!nav2_util::getCurrentPose(
      rover_pose, *tf_, goals.front().header.frame_id, robot_base_frame_,
      transform_tolerance_))
    {
      return BT::NodeStatus::FAILURE;
    }

    double dist_to_goal = euclidean_distance(goals.front().pose, rover_pose.pose);
    setOutput("dist_to_goal", dist_to_goal);

    // 📝 Remove passed goals
    // ❗ Note we leave one goal for the controller server to remove in FollowPath (i.e. the successful exit condition for the BT).
    while (goals.size() > 1) 
    {
      // 📝 Exit if the distance remaining to the next goal is greater than position_tolerance_.
      if (dist_to_goal > position_tolerance_) 
      {
        break;
      }

      // 📝 Exit if the angle remaining to the next goal is greater than orientation_tolerance_.
      // ❗ Use our custom function instead of angles::shortest_angular_distance
      double rover_yaw = tf2::getYaw(rover_pose.pose.orientation);
      double goal_yaw = tf2::getYaw(goals.front().pose.orientation);
      double angle_to_goal = utils::nav2::shortestAngularDistance(rover_yaw, goal_yaw);
      if (std::fabs(angle_to_goal) > orientation_tolerance_) 
      {
        break;
      }

      // 📝 Remove the next goal
      goals.erase(goals.begin());
      RCLCPP_INFO(
        node_->get_logger(), "Goal reached! Removing goal."
      );
    }

    setOutput("output_goals", goals);

    return BT::NodeStatus::SUCCESS;
  }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::RemovePassedURCGoalsAction>("RemovePassedURCGoals");
}