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
 * once the rover is within the search radius.
 * 
 * @authors Terry Tian
 */

#include <string>
#include <memory>

#include "nav2_util/geometry_utils.hpp"
#include "nav2_util/robot_utils.hpp"
#include "rclcpp/logging.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "tf2/LinearMath/Vector3.h"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "tf2/utils.h"
#include "nav2_behavior_tree/bt_utils.hpp"

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

    void PlaceSearchGoalsAction::initialize()
    {
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
        getInput("goal_type", goal_type_);
        getInput("search_radius", search_radius_);
        getInput("edge_offset", edge_offset_);
        
        initialized_ = true;
    }
    
    inline BT::NodeStatus PlaceSearchGoalsAction::tick()
    {
        if (!initialized_)
        {
            initialize();
        }
        
        getInput("current_pose", current_pose_);
        getInput("input_goals", input_goals_);
        
        place_search_goals();

        return BT::NodeStatus::SUCCESS;
    }

    void PlaceSearchGoalsAction::place_search_goals()
    {
        // initial direction is the normal from the rover to the goal
        tf2::Vector3 rover;
        tf2::Vector3 goal;
        tf2::fromMsg(current_pose_.pose.position, rover);
        tf2::fromMsg(input_goals_[0].pose.position, goal);

        tf2::Vector3 dir = (goal - rover).normalized();
        for (int i = 0; i < 4; ++i)
        {
            // rotate the direction vector by 120 degrees
            double angle = utils::nav2::radians(120.0 * i);
            tf2::Vector3 rotated_dir = dir.rotate(tf2::Vector3(0, 0, 1), angle);

            // calculate the new goal's position
            tf2::Vector3 new_goal_pos = goal + (rotated_dir * (search_radius_ - edge_offset_));
            geometry_msgs::msg::PoseStamped new_goal;
            new_goal.header.frame_id = current_pose_.header.frame_id;
            new_goal.header.stamp = node_->get_clock()->now();
            tf2::toMsg(new_goal_pos, new_goal.pose.position);
            // set the new goal's orientation
            tf2::Quaternion q;
            q.setRPY(0, 0, angle);
            new_goal.pose.orientation = tf2::toMsg(q);

            input_goals_.push_back(new_goal);
        }

        RCLCPP_INFO(node_->get_logger(), "Placed search goals in a %f m radius", search_radius_);

        setOutput("output_goals", input_goals_);
    }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::PlaceSearchGoalsAction>("PlaceSearchGoals");
}