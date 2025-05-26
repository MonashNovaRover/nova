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

#include <string>
#include <memory>
#include <limits>
#include <vector>
#include <cmath>
#include <utility>

#include "nav2_util/geometry_utils.hpp"
#include "rclcpp/logging.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/point.hpp"
#include "tf2/LinearMath/Vector3.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "tf2/utils.h"

#include "nova_behavior_tree/action/update_goals_action.hpp"
#include "nova_behavior_tree/nav2_utils.hpp"

namespace nova_behavior_tree
{

    using namespace nav2_util::geometry_utils;

    UpdateGoalsAction::UpdateGoalsAction(
    const std::string & name,
    const BT::NodeConfiguration & conf)
    : BT::ActionNodeBase(name, conf),
    viapoint_overwrite_tolerance_(0.5)
    {
    }

    void UpdateGoalsAction::initialize()
    {
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
        
        getInput("update_radius", viapoint_overwrite_tolerance_);
        getInput("goal_radius", goal_radius_);
        getInput("goal_type", goal_type_);

        double footprint_radius;
        if (!node_->get_parameter_or("local_costmap.local_costmap.robot_radius", footprint_radius, 0.85))
        {
            RCLCPP_ERROR(node_->get_logger(), "Failed to get local footprint, using default value of 0.85m");
        }
        goal_radius_ += footprint_radius;
        
        initialized_ = true;
    }

    inline BT::NodeStatus UpdateGoalsAction::tick()
    {
        if (!initialized_)
        {
            initialize();
        }
        
        getInput("current_pose", current_pose_);
        getInput("goals", goals_);
        size_t input_goals_count = input_goals_.size();
        getInput("input_goals", input_goals_);
        int removed_goals_count = input_goals_count - input_goals_.size();

        update_goals(removed_goals_count);

        return BT::NodeStatus::SUCCESS;
    }

    void UpdateGoalsAction::update_goals(size_t removed_goals_count)
    {
        // update prev_cube_goals_ in case goals have been removed
        for (size_t i = 0; i < prev_cube_goals_.size();)
        {
            prev_cube_goals_[i].index -= removed_goals_count;
            if (prev_cube_goals_[i].index < 0)
            {
                prev_cube_goals_.erase(prev_cube_goals_.begin() + i);
            }
            else
            {
                i += 1;
            }
        }

        for (size_t i = 0; i < goals_.size(); ++i)
        {
            // calculate offset goal so rover doesn't try to path through an object
            tf2::Vector3 rover;
            tf2::Vector3 goal;
            tf2::fromMsg(current_pose_.pose.position, rover);
            tf2::fromMsg(goals_[i].pose.position, goal);
            
            tf2::Vector3 rover_to_goal_normal = (goal - rover).normalized();
            tf2::Vector3 offset_position = goal - rover_to_goal_normal * goal_radius_;

            geometry_msgs::msg::PoseStamped offset_goal;
            offset_goal.header = goals_[i].header;
            tf2::toMsg(offset_position, offset_goal.pose.position);
            utils::nav2::orientTowards(offset_goal.pose, goals_[i].pose.position);

            auto log_goal_info = [&]()
            {
                RCLCPP_INFO(
                    node_->get_logger(),
                    "Offset goal: (%.2f, %.2f, %.2f) Cube position: (%.2f, %.2f, %.2f)\n"
                    "Rover orientation: %d° Goal orientation: %d°",
                    offset_goal.pose.position.x, offset_goal.pose.position.y, offset_goal.pose.position.z,
                    goals_[i].pose.position.x, goals_[i].pose.position.y, goals_[i].pose.position.z,
                    (int)std::round(utils::nav2::degrees(tf2::getYaw(current_pose_.pose.orientation))),
                    (int)std::round(utils::nav2::degrees(tf2::getYaw(offset_goal.pose.orientation)))
                );
            };

            // check if goal already exists
            bool updated = false;
            for (auto &prev_goal : prev_cube_goals_)
            {
                if (euclidean_distance(prev_goal.pose, goals_[i].pose) < viapoint_overwrite_tolerance_)
                {
                    if (!utils::nav2::arePointsEqual(prev_goal.pose.position, goals_[i].pose.position))
                    {
                        input_goals_[prev_goal.index] = offset_goal;
                        prev_goal.pose = goals_[i].pose;
                        RCLCPP_INFO(node_->get_logger(), "Updating existing %s goal", goal_type_.c_str());
                        log_goal_info();
                    }
                    updated = true;
                    break;
                }
            }

            if (updated)
            {
                continue;
            }

            // goal doesn't exist, insert goal
            double dist_to_rover = euclidean_distance(current_pose_.pose, goals_[i].pose);
            size_t j = 0;
            while (j < input_goals_.size() && dist_to_rover > euclidean_distance(current_pose_.pose, input_goals_[j].pose))
            {
                j += 1;
            }

            input_goals_.insert(input_goals_.begin() + j, offset_goal);
            prev_cube_goals_.emplace_back(GoalEntry{goals_[i].pose, static_cast<int>(j)});
            RCLCPP_INFO(node_->get_logger(), "Inserting new %s goal", goal_type_.c_str());
            log_goal_info();
        }

        setOutput("cube_goal_entries", prev_cube_goals_);
        setOutput("output_goals", input_goals_);
    }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::UpdateGoalsAction>("UpdateGoals");
}