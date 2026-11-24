// Copyright (c) 2025 Monash Nova Rover
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
 
#ifndef NOVA_BEHAVIOR_TREE__ACTION__CHECK_PATH_LENGTH_HPP_
#define NOVA_BEHAVIOR_TREE__ACTION__CHECK_PATH_LENGTH_HPP_
 
#include <string>
 
#include "behaviortree_cpp/action_node.h"
#include "behaviortree_cpp/bt_factory.h"
#include "nav_msgs/msg/path.hpp"
 
namespace nova_behavior_tree
{
 
/**
 * @brief Checks that a planned path is not longer than a given limit.
 *
 * Ports:
 *   - input  "path"       (nav_msgs::msg::Path)  the path to measure
 *   - input  "max_length" (double, metres)       the longest acceptable path
 *   - output "length"     (double, metres)       the measured path length
 *
 * Returns SUCCESS if the path is non-empty and its length is within
 * max_length. Returns FAILURE if the path is empty, too long, or an
 * input port could not be read.
 */
class CheckPathLength : public BT::SyncActionNode
{
public:
  CheckPathLength(
    const std::string & name,
    const BT::NodeConfiguration & config);
 
  static BT::PortsList providedPorts();
 
  BT::NodeStatus tick() override;
 
  /**
   * @brief Sum of the 2D distances between consecutive poses in a path.
   *
   * Public and static so it can be unit-tested without building a tree.
   */
  static double computePathLength(const nav_msgs::msg::Path & path);
};
 
}  // namespace nova_behavior_tree
 
#endif  // NOVA_BEHAVIOR_TREE__ACTION__CHECK_PATH_LENGTH_HPP_
 