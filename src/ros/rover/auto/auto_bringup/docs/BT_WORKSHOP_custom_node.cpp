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
 
#include "nova_behavior_tree/action/check_path_length.hpp"
 
#include <cmath>
#include <iostream>
#include <string>
 
namespace nova_behavior_tree
{
 
CheckPathLength::CheckPathLength(
  const std::string & name,
  const BT::NodeConfiguration & config)
: BT::SyncActionNode(name, config)
{
}
 
BT::PortsList CheckPathLength::providedPorts()
{
  return {
    BT::InputPort<nav_msgs::msg::Path>("path", "Path to measure"),
    BT::InputPort<double>("max_length", 50.0, "Maximum allowed path length (m)"),
    BT::OutputPort<double>("length", "Measured path length (m)")
  };
}
 
BT::NodeStatus CheckPathLength::tick()
{
  // 1. Read the inputs. getInput returns BT::Expected<T>, which converts to
  //    false if the port is missing or could not be converted.
  auto path = getInput<nav_msgs::msg::Path>("path");
  if (!path) {
    std::cerr << "[CheckPathLength] '" << name() << "' could not read 'path': " <<
      path.error() << std::endl;
    return BT::NodeStatus::FAILURE;
  }
 
  auto max_length = getInput<double>("max_length");
  if (!max_length) {
    std::cerr << "[CheckPathLength] '" << name() << "' could not read 'max_length': " <<
      max_length.error() << std::endl;
    return BT::NodeStatus::FAILURE;
  }
 
  // 2. An empty path is not a usable plan, so treat it as a failure.
  if (path.value().poses.empty()) {
    setOutput("length", 0.0);
    std::cerr << "[CheckPathLength] '" << name() << "' received an empty path" <<
      std::endl;
    return BT::NodeStatus::FAILURE;
  }
 
  // 3. Measure the path and publish the result for other nodes to use.
  const double length = computePathLength(path.value());
  setOutput("length", length);
 
  // 4. Decide the status.
  if (length > max_length.value()) {
    std::cout << "[CheckPathLength] '" << name() << "' path too long: " << length <<
      " m > " << max_length.value() << " m" << std::endl;
    return BT::NodeStatus::FAILURE;
  }
 
  return BT::NodeStatus::SUCCESS;
}
 
double CheckPathLength::computePathLength(const nav_msgs::msg::Path & path)
{
  double length = 0.0;
  for (size_t i = 1; i < path.poses.size(); ++i) {
    const auto & prev = path.poses[i - 1].pose.position;
    const auto & curr = path.poses[i].pose.position;
    length += std::hypot(curr.x - prev.x, curr.y - prev.y);
  }
  return length;
}
 
}  // namespace nova_behavior_tree
 
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::CheckPathLength>("CheckPathLength");
}