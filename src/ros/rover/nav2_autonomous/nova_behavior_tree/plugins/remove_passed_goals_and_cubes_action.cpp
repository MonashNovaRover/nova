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
#include <sstream>
#include <vector>

#include "nav_msgs/msg/path.hpp"
#include "nav2_util/geometry_utils.hpp"
#include "rclcpp/logging.hpp"
#include "tf2/utils.h"

#include "nova_behavior_tree/remove_passed_goals_and_cubes_action.hpp"
#include "nova_behavior_tree/nav2_utils.hpp"
#include "nova_behavior_tree/utils.hpp"

namespace nova_behavior_tree
{

    RemovePassedGoalsAndCubesAction::RemovePassedGoalsAndCubesAction(
        const std::string & name,
        const BT::NodeConfiguration & conf)
        : BT::ActionNodeBase(name, conf),
        viapoint_achieved_radius_(0.5)
    {
    }

    void RemovePassedGoalsAndCubesAction::initialize()
    {
        tf_ = config().blackboard->get<std::shared_ptr<tf2_ros::Buffer>>("tf_buffer");
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
        node_->get_parameter("transform_tolerance", transform_tolerance_);
        robot_base_frame_ = BT::deconflictPortAndParamFrame<std::string>(node_, "robot_base_frame", this);
        
        getInput("radius", viapoint_achieved_radius_);
        getInput("cube_tolerance", cube_tolerance_);
        getInput("yaw_tolerance", yaw_tolerance_);

        initialized_ = true;
    }

    inline BT::NodeStatus RemovePassedGoalsAndCubesAction::tick()
    {
        if (!initialized_)
        {
            initialize();
        }

        Goals goal_poses;
        getInput("input_goals", goal_poses);

        if (goal_poses.empty()) 
        {
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
        double dist_to_cube;
        geometry_msgs::msg::PoseStamped cube;
        getInput("input_goal", cube);

        while (goal_poses.size() > 1) 
        {
            dist_to_goal = euclidean_distance(goal_poses[0].pose, current_pose.pose);

            if (dist_to_goal > viapoint_achieved_radius_) 
            {
                break;
            }

            // Orientation check
            double current_yaw = tf2::getYaw(current_pose.pose.orientation);
            double goal_yaw = tf2::getYaw(goal_poses[0].pose.orientation);

            // Use our custom function instead of angles::shortest_angular_distance
            double dyaw = utils::nav2::shortestAngularDistance(current_yaw, goal_yaw);

            if (std::fabs(dyaw) > yaw_tolerance_) 
            {
                // Orientation mismatch, break
                break;
            }

            dist_to_cube = euclidean_distance(goal_poses[0].pose, cube.pose);

            if (dist_to_cube < cube_tolerance_) 
            {
                int id;
                getInput("id", id);

                RCLCPP_INFO(node_->get_logger(), "New cube visited: %s", COLORS[id].c_str());

                visited_ids_[id] = true;
                setOutput("visited_ids", visited_ids_);
                visited_colors_.push_back(COLORS[id]);

                RCLCPP_INFO(
                    node_->get_logger(), "Visited cubes: %s",
                    utils::vectorToString<std::string>(visited_colors_).c_str()
                );
            }

            goal_poses.erase(goal_poses.begin());
        }

        setOutput("output_goals", goal_poses);

        return BT::NodeStatus::SUCCESS;
    }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::RemovePassedGoalsAndCubesAction>("RemovePassedGoalsAndCubes");
}