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
 * are added as viapoints in UpdateGoals, and saved to a list for future reference.
 * 
 * @authors Terry Tian
 */

#include <string>
#include <vector>
#include <memory>
#include <limits>
#include <algorithm>
#include <chrono>
#include <thread>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "nav2_util/geometry_utils.hpp"
#include "nav2_util/robot_utils.hpp"
#include "rclcpp/logging.hpp"
#include "nav2_behavior_tree/bt_utils.hpp"

#include "nova_behavior_tree/condition/cube_detected_condition.hpp"
#include "nova_behavior_tree/nav2_utils.hpp"

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

        initialized_ = true;
    }

    BT::NodeStatus CubeDetectedCondition::tick()
    {
        if (!initialized_)
        {
            initialize();
            // !!! DO NOT REMOVE, IT WILL NOT WORK OTHERWISE IDK WHY AAAAAAAA
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

        bool result = false;
        geometry_msgs::msg::TransformStamped t;
        Goal current_pose;
        Goals goals;
        goals.reserve(4);
        
        IDs visited_ids;
        getInput("visited_ids", visited_ids);

        // get current robot pose
        if (!nav2_util::getCurrentPose(current_pose, *tf_, global_frame_, robot_base_frame_, transform_tolerance_))
        {
            RCLCPP_ERROR(node_->get_logger(), "Failed to get current pose of rover");
            return false;
        }
        
        setOutput("current_pose", current_pose);

        // query cube poses
        for (size_t i = 0; i < 4; i++)
        {
            if (visited_ids[i])
            {
                continue;
            }

            std::string child_frame = COLORS[i] + "_obj";
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

            Goal goal;

            cube_pose.position.x = t.transform.translation.x;
            cube_pose.position.y = t.transform.translation.y;
            cube_pose.position.z = t.transform.translation.z;
            cube_pose.orientation = t.transform.rotation;

            if (utils::nav2::arePointsEqual(cube_poses_[i].position, cube_pose.position))
            {
                continue;
            }
            
            cube_poses_[i] = cube_pose;
            
            RCLCPP_INFO(node_->get_logger(), "%s cube detected", COLORS[i].c_str());
            
            goal.header.frame_id = global_frame_;
            goal.header.stamp = t.header.stamp;
            goal.pose = cube_pose;
            
            goals.push_back(goal);

            result = true;
        }

        setOutput("cube_poses", cube_poses_);
        setOutput("cube_goals", goals);

        return result;
    }

}

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
    factory.registerNodeType<nova_behavior_tree::CubeDetectedCondition>("CubeDetected");
}