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
#include <cmath>

#include "nova_behavior_tree/remove_passed_goals_and_ar_tags_action.hpp"

namespace nova_behavior_tree
{

    // Small helper to replicate angles::shortest_angular_distance
    inline double shortestAngularDistance(double from, double to)
    {
    // Normalizes the difference into (-π, +π]
    double angle = std::fmod(to - from, 2.0 * M_PI);
    if (angle > M_PI) {
        angle -= 2.0 * M_PI;
    } else if (angle <= -M_PI) {
        angle += 2.0 * M_PI;
    }
    return angle;
    }

    RemovePassedGoalsAndARTagsAction::RemovePassedGoalsAndARTagsAction(
        const std::string & name,
        const BT::NodeConfiguration & conf)
        : BT::ActionNodeBase(name, conf),
        viapoint_achieved_radius_(0.5)
    {
    }

    void RemovePassedGoalsAndARTagsAction::initialize()
    {
        getInput("radius", viapoint_achieved_radius_);
        getInput("tag_tolerance", tag_tolerance_);

        tf_ = config().blackboard->get<std::shared_ptr<tf2_ros::Buffer>>("tf_buffer");
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
        node_->get_parameter("transform_tolerance", transform_tolerance_);
        robot_base_frame_ = BT::deconflictPortAndParamFrame<std::string>(
        node_, "robot_base_frame", this);
    }

    inline BT::NodeStatus RemovePassedGoalsAndARTagsAction::tick()
    {
        if (!BT::isStatusActive(status())) 
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
        double dist_to_tag;
        geometry_msgs::msg::PoseStamped tag;
        getInput("input_goal", tag);

        // Define orientation tolerance (~14 degrees)
        double yaw_goal_tolerance = 0.25;  // radians

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
            double dyaw = shortestAngularDistance(current_yaw, goal_yaw);

            if (std::fabs(dyaw) > yaw_goal_tolerance) 
            {
                // Orientation mismatch, break
                break;
            }

            dist_to_tag = euclidean_distance(goal_poses[0].pose, tag.pose);

            if (dist_to_tag < tag_tolerance_) 
            {
                IDs seen_ids;
                getInput("seen_ids", seen_ids);                
                int id;
                getInput("id", id);

                RCLCPP_INFO(node_->get_logger(), "New ID %i being added to seen_ids: %s", id, vectorToString(seen_ids).c_str());

                seen_ids.push_back(id);
                setOutput("seen_ids", seen_ids);

                RCLCPP_INFO(node_->get_logger(), "seen_ids: %s", vectorToString(seen_ids).c_str());
            }

            goal_poses.erase(goal_poses.begin());
        }

        setOutput("output_goals", goal_poses);

        return BT::NodeStatus::SUCCESS;
    }



    std::string RemovePassedGoalsAndARTagsAction::vectorToString(const std::vector<int>& vec) 
    {
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < vec.size(); ++i) {
            oss << vec[i];
            if (i < vec.size() - 1) {
                oss << ", ";
            }
        }
        oss << "]";
        return oss.str();
    }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::RemovePassedGoalsAndARTagsAction>("RemovePassedGoalsAndARTags");
}