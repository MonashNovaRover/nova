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
 * @brief Action node for saving cube poses to a file. To avoid having to rewrite
 * potentially huge amounts of data every time, the poses are appended to file instead.
 * 
 * @authors Terry Tian
 */

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "rclcpp/logging.hpp"

#include "nova_behavior_tree/save_cube_poses_action.hpp"

namespace nova_behavior_tree
{

    SaveCubePosesAction::SaveCubePosesAction(
        const std::string & name,
        const BT::NodeConfiguration & conf)
        : BT::SyncActionNode(name, conf)
    {
    }

    void SaveCubePosesAction::initialize()
    {
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
        
        // these shouldn't change in lifecycle of BT
        getInput("file_path", file_path_);
        getInput("cube_poses", cube_poses_);

        // avoid overwriting existing files
        std::ifstream file(file_path_);
        std::string og_file_path = file_path_;
        size_t i = 1;
        while (file.good())
        {
            std::ostringstream oss;
            oss << og_file_path << "(" << i << ")";
            file_path_ = oss.str();
            file.open(file_path_);
            i += 1;
        }
        
        initialized_ = true;
    }

    inline BT::NodeStatus SaveCubePosesAction::tick()
    {
        if (!initialized_)
        {
            initialize();
        }
        
        file_.open(file_path_, std::ios::app);

        if (!file_.is_open())
        {
            RCLCPP_ERROR(node_->get_logger(), "Failed to open file %s", file_path_.c_str());
            return BT::NodeStatus::FAILURE;
        }

        file_ << CubePosesToString(*cube_poses_, start_points_);
        RCLCPP_INFO(node_->get_logger(), "Saved cube poses to %s", file_path_.c_str());

        file_.close();

        return BT::NodeStatus::SUCCESS;
    }

    std::string SaveCubePosesAction::CubePosesToString(const CubePoses& cube_poses, size_t (&start_points)[4])
    {
        std::ostringstream oss;
        
        for (size_t i = 0; i < 4; ++i)
        {
            oss << COLORS[i] << ": ";
            if (cube_poses[i].size() > start_points[i])
            {
                for (size_t j = start_points[i]; j < cube_poses[i].size() - 1; j++)
                {
                    const auto& pose = cube_poses[i][j];
                    oss << "(" << pose.position.x << ", " << pose.position.y << ", " 
                        << pose.position.z << "), ";
                }
                const auto& pose = cube_poses[i].back();
                oss << "(" << pose.position.x << ", " << pose.position.y << ", " 
                    << pose.position.z << ")";
                
                start_points[i] = cube_poses[i].size();
            }
            oss << '\n';
        }
        oss << '\n';

        return oss.str();
    }

}

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
    factory.registerNodeType<nova_behavior_tree::SaveCubePosesAction>("SaveCubePoses");
}