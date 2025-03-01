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
 * If the file already exists, the file name is incremented to avoid overwriting.
 * e.g. poses.txt -> poses_1.txt -> poses_2.txt -> ...
 * 
 * @authors Terry Tian
 */

#ifndef NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__SAVE_CUBE_POSES_ACTION_HPP_
#define NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__SAVE_CUBE_POSES_ACTION_HPP_

#include <string>
#include <array>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>

#include "geometry_msgs/msg/pose.hpp"
#include "rclcpp/rclcpp.hpp"
#include "behaviortree_cpp/action_node.h"

namespace nova_behavior_tree
{

  class SaveCubePosesAction : public BT::SyncActionNode
  {
  public:
    typedef std::array<std::vector<geometry_msgs::msg::Pose>, 4> CubePoses;

    /**
     * @brief A constructor for nova_behavior_tree::ARTagDetectedCondition
     * @param condition_name Name for the XML tag for this node
     * @param conf BT node configuration
     */
    SaveCubePosesAction(
        const std::string &xml_tag_name,
        const BT::NodeConfiguration &conf);

    /**
     * @brief Function to initialize variables,
     * called only once in the lifecycle of the BT
     */
    void initialize();

    /**
     * @brief The main override required by a BT node
     * @return BT::NodeStatus Status of tick execution
     */
    BT::NodeStatus tick() override;

    /**
     * @brief Creates list of BT ports
     * @return BT::PortsList Containing node-specific ports
     */
    static BT::PortsList providedPorts()
    {
      return {
          BT::InputPort<std::shared_ptr<CubePoses>>("cube_poses", "List of cube poses"),
          BT::InputPort<std::string>("file_path", "File to save cube poses to"),
      };
    }

  private:
    std::string CubePosesToString(const CubePoses &cube_poses, size_t (&start_points)[4]);

    rclcpp::Node::SharedPtr node_;

    std::shared_ptr<CubePoses> cube_poses_;
    std::string file_path_;
    std::ofstream file_;
    
    bool initialized_ = false;
    size_t start_points_[4] = {};
    const std::string COLORS[4] = {"red", "green", "blue", "white"};
  };

}

#endif // NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__SAVE_CUBE_POSES_ACTION_HPP_