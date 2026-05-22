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

/**
 * @brief Places search goals for AR Tags and Objects related to the URC mission
 * within the search radius in a spiral pattern.
 * 
 * @authors Terry Tian, Harry Mills
 */

#include <string>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/logging.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "tf2/LinearMath/Vector3.hpp"
#include "tf2/LinearMath/Quaternion.hpp"
#include "tf2/utils.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

#include "nova_behavior_tree/action/place_search_goals_action.hpp"
#include "nova_behavior_tree/nav2_utils.hpp"

namespace nova_behavior_tree
{

  PlaceSearchGoalsAction::PlaceSearchGoalsAction(
  const std::string & name,
  const BT::NodeConfiguration & conf)  
  : BT::ActionNodeBase(name, conf)
  {
  }
  const double pi = std::acos(-1.0);


  void PlaceSearchGoalsAction::initialize()
  {
      node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
      getInput("search_radius", search_radius_);
      getInput("search_corners", search_corners_);
      getInput("search_spacing", search_spacing_);
      
      initialized_ = true;
  }
    
  inline BT::NodeStatus PlaceSearchGoalsAction::tick()
  {
    if (!initialized_)
    {
      initialize();
    }
      
    getInput("input_goals", input_goals_);
    place_search_path();

    return BT::NodeStatus::SUCCESS;
  }

  void PlaceSearchGoalsAction::place_search_path()
  {
    Goal centre_goal = input_goals_.back();

    input_goals_.push_back(place_goal(centre_goal, 0, search_spacing_/2));

    RCLCPP_INFO(node_->get_logger(), "Commencing placing search goals with R: %f | S: %2.2f | C: %d", search_radius_, search_spacing_, search_corners_);


    double loops = search_radius_ / search_spacing_;
    double points = static_cast<int>((0.5 + loops) * search_corners_);

    for (int i = search_corners_/2; i < points; ++i)
    {
      double angle = ((2*pi) / search_corners_) * (i % search_corners_) - pi/2;
      double dist = (static_cast<double>(i) / search_corners_) * search_spacing_;

      input_goals_.push_back(place_goal(centre_goal, angle, dist));
    }

    RCLCPP_INFO(node_->get_logger(), "Placed search goals in a %f m radius", search_radius_);

    setOutput("output_goals", input_goals_);
  }

  geometry_msgs::msg::PoseStamped PlaceSearchGoalsAction::place_goal(const geometry_msgs::msg::PoseStamped& centre_goal, double angle, double dist)
  {
    //get centre goal properties
    tf2::Vector3 centre;
    tf2::fromMsg(centre_goal.pose.position, centre);
    double yaw = tf2::getYaw(centre_goal.pose.orientation);
    tf2::Vector3 dir = tf2::Vector3(std::cos(yaw), std::sin(yaw), 0);

    // calculate the new goal's position
    tf2::Vector3 rotated_dir = dir.rotate(tf2::Vector3(0, 0, 1), angle);
    tf2::Vector3 new_goal_pos = centre + (rotated_dir * dist);

    //create goal
    geometry_msgs::msg::PoseStamped new_goal;
    new_goal.header.frame_id = centre_goal.header.frame_id;
    new_goal.header.stamp = node_->get_clock()->now();
    tf2::toMsg(new_goal_pos, new_goal.pose.position);

    // set the new goal's orientation to be perpendicular from radial direction
    tf2::Quaternion q;

    q.setRPY(0, 0, yaw + angle + pi/2);
    new_goal.pose.orientation = tf2::toMsg(q);
    
    return new_goal;
  }
    
}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::PlaceSearchGoalsAction>("PlaceSearchGoals");
}