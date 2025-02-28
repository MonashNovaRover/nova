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
 * @brief Condition node for checking for detected cubes. Detected cubes' poses
 * are added as goals to the front of the goals list, and saved to a list for
 * future reference.
 * 
 * The filter keeps track of the past n detections, determined by filter_tolerance.
 * It's implemented as a queue of ints in the form of binary literals, e.g. 0b1010.
 * Each bit represents whether that cube has been detected.
 * 
 * e.g. 0b   0    1     0    1
 *          red green blue white
 * 
 * @authors Terry Tian
 */

#include <string>
#include <memory>
#include <limits>
#include <queue>
#include <algorithm>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "nav2_util/geometry_utils.hpp"
#include "nav2_util/robot_utils.hpp"
#include "rclcpp/logging.hpp"
#include "nav2_behavior_tree/bt_utils.hpp"

#include "nova_behavior_tree/nav2_utils.hpp"
#include "nova_behavior_tree/cube_detected_condition.hpp"

namespace nova_behavior_tree
{

    CubeDetectedCondition::CubeDetectedCondition(
        const std::string &condition_name,
        const BT::NodeConfiguration &conf)
        : BT::ConditionNode(condition_name, conf)
    {
    }

    void CubeDetectedCondition::initialize()
    {
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
        node_->get_parameter("transform_tolerance", transform_tolerance_);
        
        global_frame_ = BT::deconflictPortAndParamFrame<std::string>(node_, "global_frame", this);
        robot_base_frame_ = BT::deconflictPortAndParamFrame<std::string>(node_, "robot_base_frame", this);

        tf_ = config().blackboard->get<std::shared_ptr<tf2_ros::Buffer>>("tf_buffer");
        tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_);

        cube_poses_ = std::make_shared<CubePoses>();
        setOutput("cube_poses", cube_poses_);

        getInput("filter_strength", filter_strength_);
        getInput("filter_tolerance", filter_tolerance_);
        // just in case filter_tolerance is less than filter_strength
        filter_tolerance_ = std::max(filter_tolerance_, filter_strength_);

        for (size_t i = 0; i < (unsigned int)filter_tolerance_; i++)
        {
            filter_.push(0b0000);
        }

        initialized_ = true;
    }

    BT::NodeStatus CubeDetectedCondition::tick()
    {
        if (!initialized_)
        {
            initialize();
        }

        if (detected())
        {
            return BT::NodeStatus::SUCCESS;
        }

        return BT::NodeStatus::FAILURE;
    }

    bool CubeDetectedCondition::detected()
    {
        using namespace nav2_util::geometry_utils;

        geometry_msgs::msg::TransformStamped t;
        geometry_msgs::msg::PoseStamped goal;
        geometry_msgs::msg::PoseStamped current_pose;
        double dist_to_goal = std::numeric_limits<double>::infinity();
        int cube_id = -1;
        int detections = 0b0000;
        
        IDs visited_ids;
        getInput("visited_ids", visited_ids);

        // get current robot pose
        if (!nav2_util::getCurrentPose(current_pose, *tf_, global_frame_, robot_base_frame_, transform_tolerance_))
        {
            RCLCPP_ERROR(node_->get_logger(), "Failed to get current pose of rover");
            return false;
        }

        // query cube poses
        for (size_t i = 0; i < 4; i++)
        {
            if (visited_ids[i] || filter_detections_count_[i] < filter_strength_)
            {
                continue;
            }

            std::string child_frame = COLORS[i] + "_cube";
            geometry_msgs::msg::Pose cube_pose;

            try
            {
                t = tf_->lookupTransform(
                    global_frame_, child_frame,
                    tf2::TimePointZero
                );
            }
            catch (tf2::TransformException &ex)
            {
                continue;
            }

            cube_pose.position.x = t.transform.translation.x;
            cube_pose.position.y = t.transform.translation.y;
            cube_pose.position.z = t.transform.translation.z;
            cube_pose.orientation = t.transform.rotation;
            
            if ((*cube_poses_)[i].empty() || 
                !utils::nav2::arePointsEqual((*cube_poses_)[i].back().position, cube_pose.position))
            {
                (*cube_poses_)[i].push_back(cube_pose);
            }
            
            RCLCPP_INFO(node_->get_logger(), "%s cube detected, added pose to cube_poses", COLORS[i].c_str());

            double dist_to_cube = euclidean_distance(cube_pose, current_pose.pose);
            if (dist_to_cube < dist_to_goal)
            {
                goal.header.frame_id = global_frame_;
                goal.header.stamp = t.header.stamp;
                goal.pose = cube_pose;
                dist_to_goal = dist_to_cube;
                cube_id = i;
            }
            
            detections |= 1 << (3 - i);
            filter_detections_count_[i]++;
        }

        // if no cube is detected currently, retrieve nearest last seen cube that has not been visited
        if (cube_id == -1)
        {
            for (size_t i = 0; i < 4; i++)
            {
                if (visited_ids[i] || (*cube_poses_)[i].empty())
                {
                    continue;
                }
                
                geometry_msgs::msg::Pose cube_pose = (*cube_poses_)[i].back();
                double dist_to_cube = euclidean_distance(cube_pose, current_pose.pose);
                if (dist_to_cube < dist_to_goal)
                {
                    goal.header.frame_id = global_frame_;
                    goal.pose = cube_pose;
                    dist_to_goal = dist_to_cube;
                    cube_id = i;
                }
            }
        }

        // update filter
        if (!filter_.empty())
        {
            filter_.push(detections);
            int front = filter_.front();
            filter_.pop();

            for (size_t i = 4; i > 0; i--)
            {
                filter_detections_count_[i] -= front & 1;
                front >>= 1;
            }
        }

        if (cube_id != -1)
        {
            setOutput("goal", goal);
            setOutput("id", cube_id);
            RCLCPP_INFO(node_->get_logger(), "Pathing towards %s cube", COLORS[cube_id].c_str());
            return true;
        }

        return false;
    }

}

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
    factory.registerNodeType<nova_behavior_tree::CubeDetectedCondition>("CubeDetected");
}