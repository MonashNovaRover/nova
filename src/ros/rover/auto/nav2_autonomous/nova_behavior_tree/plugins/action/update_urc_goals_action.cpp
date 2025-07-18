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
#include <vector>

#include "rclcpp/logging.hpp"
#include "nav2_util/geometry_utils.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "tf2/LinearMath/Vector3.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "tf2/utils.h"
#include "nav2_behavior_tree/bt_utils.hpp"

#include "nova_behavior_tree/action/update_urc_goals_action.hpp"
#include "nova_behavior_tree/nav2_utils.hpp"

namespace nova_behavior_tree
{

  UpdateURCGoalsAction::UpdateURCGoalsAction(
  const std::string & name,
  const BT::NodeConfiguration & conf)
  : BT::ActionNodeBase(name, conf)
  {
  }

  void UpdateURCGoalsAction::initialize()
  {
    node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
    
    offset_radius_ = BT::deconflictPortAndParamFrame<double>(node_, "goal_offset_radius", this);
    RCLCPP_INFO(node_->get_logger(), "Offset radius: %f", offset_radius_);
    
    double footprint_radius;
    if (!node_->get_parameter_or("robot_radius", footprint_radius, 0.85))
    {
        RCLCPP_ERROR(node_->get_logger(), "Failed to get local footprint, using default value of 0.85m");
    }
    offset_radius_ += footprint_radius;
    
    initialized_ = true;
  }

  inline BT::NodeStatus UpdateURCGoalsAction::tick()
  {
    // 📝 Initialise the node on startup with static inputs
    if (!initialized_)
    {
        initialize();
    }

    // 📝 Get new dynamic inputs
    getInput("detected_goal", detected_goal_);
    getInput("current_pose", current_pose_);
    getInput("input_goals", goals_);

    update_urc_goals();

    return BT::NodeStatus::SUCCESS;
  }

  void UpdateURCGoalsAction::update_urc_goals()
  {
    // remove all other goals other than the detected goal
    if (utils::nav2::isDefaultPose(prev_detected_goal_.pose))
    {
      goals_.clear();
      RCLCPP_INFO(node_->get_logger(), "First detection, removing all other goals");
    }

    // update the detected goal
    RCLCPP_INFO(node_->get_logger(), "Detected goal:\n%s", utils::nav2::poseStampedToString(detected_goal_).c_str());
    Goal offset_goal = utils::nav2::offsetGoal(detected_goal_, current_pose_, offset_radius_);
    RCLCPP_INFO(node_->get_logger(), "Offset goal:\n%s", utils::nav2::poseStampedToString(offset_goal).c_str());
    if (goals_.empty())
    {
      RCLCPP_INFO(node_->get_logger(), "Adding detected goal to goals");
      goals_.push_back(offset_goal);
    }
    else
    {
      RCLCPP_INFO(node_->get_logger(), "Updating detected goal pose");
      goals_[0] = offset_goal;
    }
    setOutput("output_goals", goals_);
    prev_detected_goal_ = detected_goal_;
  }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::UpdateURCGoalsAction>("UpdateURCGoals");
}