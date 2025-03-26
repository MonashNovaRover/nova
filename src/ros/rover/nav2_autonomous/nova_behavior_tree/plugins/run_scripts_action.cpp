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

#include <vector>
#include <string>
#include <cstdlib>

#include "rclcpp/logging.hpp"

#include "nova_behavior_tree/update_goals_action.hpp"

namespace nova_behavior_tree
{

    RunScriptsAction::RunScriptsAction(
    const std::string & name,
    const BT::NodeConfiguration & conf)
    : BT::ActionNodeBase(name, conf)
    {
    }

    void RunScriptsAction::initialize()
    {
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
        
        getInput("scripts", scripts_);
        getInput("build_path", build_path_);

        initialized_ = true;
    }

    inline BT::NodeStatus RunScriptsAction::tick()
    {
        if (!initialized_)
        {
            initialize();
        }
        
        run_scripts();

        return BT::NodeStatus::SUCCESS;
    }

    inline void RunScriptsAction::run_script()
    {
        for (const auto &script : scripts_)
        {
            system((build_path_ + script).c_str());
        }
    }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::RunScriptsAction>("RunScripts");
}