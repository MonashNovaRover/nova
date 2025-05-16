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
        getInput("ar_tag_search_radius", ar_tag_search_radius_);
        getInput("object_search_radius", object_search_radius_);
        getInput("edge_offset", edge_offset_);
        
        initialized_ = true;
    }
    
    inline BT::NodeStatus PlaceSearchGoalsAction::tick()
    {
        if (!initialized_)
        {
            initialize();
        }
        
        // getInput("current_pose", current_pose_);
        getInput("input_goals", input_goals_);
        
        // temporary until GetCurrentPose is made
        std::shared_ptr<tf2_ros::Buffer> tf = config().blackboard->get<std::shared_ptr<tf2_ros::Buffer>>("tf_buffer");
        double transform_tolerance;
        node_->get_parameter("transform_tolerance", transform_tolerance);
        std::string robot_base_frame_ = BT::deconflictPortAndParamFrame<std::string>(node_, "robot_base_frame", this);
        if (!nav2_util::getCurrentPose(
            current_pose_, *tf, input_goals_[0].header.frame_id, robot_base_frame_,
            transform_tolerance))
        {
            return BT::NodeStatus::FAILURE;
        }
        
        place_search_goals();

        return BT::NodeStatus::SUCCESS;
    }

    void PlaceSearchGoalsAction::place_search_goals()
    {
        int search_radius = goal_type_ == 1 ? ar_tag_search_radius_ : object_search_radius_;
        
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
            tf2::Vector3 new_goal_pos = goal + (rotated_dir * (search_radius - edge_offset_));
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

        setOutput("output_goals", input_goals_);

        // // update prev_cube_goals_ in case goals have been removed
        // for (size_t i = 0; i < prev_cube_goals_.size();)
        // {
        //     prev_cube_goals_[i].index -= removed_goals_count;
        //     if (prev_cube_goals_[i].index < 0)
        //     {
        //         prev_cube_goals_.erase(prev_cube_goals_.begin() + i);
        //     }
        //     else
        //     {
        //         i += 1;
        //     }
        // }

        // for (size_t i = 0; i < goals_.size(); ++i)
        // {
        //     // calculate offset goal so rover doesn't try to path through an object
        //     tf2::Vector3 rover;
        //     tf2::Vector3 goal;
        //     tf2::fromMsg(current_pose_.pose.position, rover);
        //     tf2::fromMsg(goals_[i].pose.position, goal);
            
        //     tf2::Vector3 rover_to_goal_normal = (goal - rover).normalized();
        //     tf2::Vector3 offset_position = goal - rover_to_goal_normal * goal_radius_;

        //     geometry_msgs::msg::PoseStamped offset_goal;
        //     offset_goal.header = goals_[i].header;
        //     tf2::toMsg(offset_position, offset_goal.pose.position);
        //     utils::nav2::orientTowards(offset_goal.pose, goals_[i].pose.position);

        //     auto log_goal_info = [&]()
        //     {
        //         RCLCPP_INFO(
        //             node_->get_logger(),
        //             "Offset goal: (%.2f, %.2f, %.2f) Cube position: (%.2f, %.2f, %.2f)\n"
        //             "Rover orientation: %d° Goal orientation: %d°",
        //             offset_goal.pose.position.x, offset_goal.pose.position.y, offset_goal.pose.position.z,
        //             goals_[i].pose.position.x, goals_[i].pose.position.y, goals_[i].pose.position.z,
        //             (int)std::round(utils::nav2::degrees(tf2::getYaw(current_pose_.pose.orientation))),
        //             (int)std::round(utils::nav2::degrees(tf2::getYaw(offset_goal.pose.orientation)))
        //         );
        //     };

        //     // check if goal already exists
        //     bool updated = false;
        //     for (auto &prev_goal : prev_cube_goals_)
        //     {
        //         if (euclidean_distance(prev_goal.pose, goals_[i].pose) < viapoint_overwrite_tolerance_)
        //         {
        //             if (!utils::nav2::arePointsEqual(prev_goal.pose.position, goals_[i].pose.position))
        //             {
        //                 input_goals_[prev_goal.index] = offset_goal;
        //                 prev_goal.pose = goals_[i].pose;
        //                 RCLCPP_INFO(node_->get_logger(), "Updating existing %s goal", goal_type_.c_str());
        //                 log_goal_info();
        //             }
        //             updated = true;
        //             break;
        //         }
        //     }

        //     if (updated)
        //     {
        //         continue;
        //     }

        //     // goal doesn't exist, insert goal
        //     double dist_to_rover = euclidean_distance(current_pose_.pose, goals_[i].pose);
        //     size_t j = 0;
        //     while (j < input_goals_.size() && dist_to_rover > euclidean_distance(current_pose_.pose, input_goals_[j].pose))
        //     {
        //         j += 1;
        //     }

        //     input_goals_.insert(input_goals_.begin() + j, offset_goal);
        //     prev_cube_goals_.emplace_back(GoalEntry{goals_[i].pose, static_cast<int>(j)});
        //     RCLCPP_INFO(node_->get_logger(), "Inserting new %s goal", goal_type_.c_str());
        //     log_goal_info();
        // }

        // setOutput("cube_goal_entries", prev_cube_goals_);
        // setOutput("output_goals", input_goals_);
    }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::PlaceSearchGoalsAction>("PlaceSearchGoals");
}