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
 * @brief Action node for running any scripts via the command-line.
 * Scripts to run should exclude the build path. Separate scripts by 
 * a ';' to run multiple scripts.
 * 
 * e.g. ros2 run auto_bringup script1.py;ros2 run auto_bringup script2.py
 * 
 * @authors Terry Tian, Tarik Thomas
 */

#ifndef NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__RUN_SCRIPTS_ACTION_HPP_
#define NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__RUN_SCRIPTS_ACTION_HPP_

#include <vector>
#include <string>

#include "behaviortree_cpp/action_node.h"

namespace nova_behavior_tree
{

class RunScriptsAction : public BT::ActionNodeBase
{
public:
  RunScriptsAction(
    const std::string & xml_tag_name,
    const BT::NodeConfiguration & conf);

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

  static BT::PortsList providedPorts()
  {
    return {
      BT::InputPort<std::vector<std::string>>("scripts", "Scripts to run (excluding build path)"),
      BT::InputPort<std::string>("build_path", "Build path to run the script from"),
    };
  }

private:
  void halt() override {}
  void run_scripts();

  rclcpp::Node::SharedPtr node_;
  
  std::vector<std::string> scripts_;
  std::string build_path_;
  
  bool initialized_ = false;
};

}  // namespace nova_behavior_tree

#endif  // NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__RUN_SCRIPTS_ACTION_HPP_