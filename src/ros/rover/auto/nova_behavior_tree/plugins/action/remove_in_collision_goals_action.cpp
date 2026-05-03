// Copyright (c) 2024 Angsa Robotics
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
 * @brief Action node for removing nearby goals that are in collision from the costmap
 * 
 * @authors Harry Overall
 * Last Edited: 28/4/2026
 */

#include <string>
#include <vector>
#include <array>
#include <cmath>

#include "nav2_costmap_2d/costmap_2d.hpp"
#include "nav2_util/geometry_utils.hpp"
#include "rclcpp/logging.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/point.hpp"
#include "tf2/utils.h"
#include "nav2_behavior_tree/bt_utils.hpp"
#include "nav_msgs/msg/path.hpp"
#include "behaviortree_cpp/decorator_node.h"

#include "nova_behavior_tree/action/remove_in_collision_goals_action.hpp"
#include "nova_behavior_tree/nav2_utils.hpp"
#include "rclcpp/rclcpp.hpp"

namespace nova_behavior_tree
{

using namespace nav2_util::geometry_utils;
using namespace geometry_msgs::msg;
using namespace nav_msgs::msg;

RemoveInCollisionGoalsAction::RemoveInCollisionGoalsAction(
  const std::string & name,
  const BT::NodeConfiguration & conf)
: BT::ActionNodeBase(name, conf)
{
}

void RemoveInCollisionGoalsAction::initialize()
{
  node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
  
  // Get vars for figuring out current pose
  tf_ = config().blackboard->get<std::shared_ptr<tf2_ros::Buffer>>("tf_buffer");
  node_->get_parameter("transform_tolerance", transform_tolerance_);
  global_frame_ = BT::deconflictPortAndParamFrame<std::string>(
    node_, "global_frame", this);
  robot_base_frame_ = BT::deconflictPortAndParamFrame<std::string>(
    node_, "robot_base_frame", this);

  // Get input params
  getInput("cost_threshold", cost_threshold_);

  // Subscribe to local and global costmaps via Nav2 costmap transport.
  local_costmap_sub_ = std::make_unique<nav2_costmap_2d::CostmapSubscriber>(
    node_, "/local_costmap/costmap_raw");
  global_costmap_sub_ = std::make_unique<nav2_costmap_2d::CostmapSubscriber>(
    node_, "/global_costmap/costmap_raw");

  RCLCPP_INFO(node_->get_logger(), "RemoveInCollisionGoals initialized.");
  initialized_ = true;
}

void RemoveInCollisionGoalsAction::setup()
{
  if (!initialized_)
  {
    initialize();
  }
  
  RCLCPP_INFO(node_->get_logger(), "RemoveInCollisionGoals set up.");
        
  set_up_ = true;
}

void RemoveInCollisionGoalsAction::halt()
{
  set_up_ = false;
}

inline BT::NodeStatus RemoveInCollisionGoalsAction::tick()
{
  if (!set_up_)
  {
      setup();
  }

  if (!have_costmaps())
  {
      RCLCPP_WARN_THROTTLE(
        node_->get_logger(), *node_->get_clock(), 2000,
        "RemoveInCollisionGoals waiting for local/global costmaps.");
      return BT::NodeStatus::RUNNING;
  }

  getInput("input_goals", input_goals_);

  // remove goals
  if (remove_goals())
  {
      return BT::NodeStatus::SUCCESS;
  }

  // On failure, just return input goals
  RCLCPP_ERROR(node_->get_logger(), "RemoveInCollisionGoals Failed remove to goals, goals remain unchanged.");
  setOutput("output_goals", input_goals_);
  return BT::NodeStatus::FAILURE;
}

bool RemoveInCollisionGoalsAction::have_costmaps()
{
  // This BT plugin uses a blackboard node that may not always be serviced by
  // the same executor thread cadence as Nav2 servers, so process pending
  // subscription callbacks before checking costmap availability.
  rclcpp::spin_some(node_);

  try
  {
    local_costmap_ = local_costmap_sub_->getCostmap();
    global_costmap_ = global_costmap_sub_->getCostmap();
  }
  catch (const std::runtime_error &)
  {
    local_costmap_.reset();
    global_costmap_.reset();
    return false;
  }

  return static_cast<bool>(local_costmap_) && static_cast<bool>(global_costmap_);
}


bool RemoveInCollisionGoalsAction::remove_goals()
{
  // Get the current pose of the rover
  geometry_msgs::msg::PoseStamped current_pose;
  
  if (!nav2_util::getCurrentPose(current_pose, *tf_, global_frame_, robot_base_frame_, transform_tolerance_))
  {
    RCLCPP_WARN(node_->get_logger(), "Current robot pose is not available.");
    return false;
  }
  
  Goals output_goals_;
  for (size_t i=0; i < input_goals_.size(); i++)
  {
    Goal goal = input_goals_[i];
    
    // Check if costmap where goal is at is too high
    if (!is_goal_in_collision(goal))
    {
      output_goals_.push_back(goal);
    }
    else 
    {
      RCLCPP_INFO(node_->get_logger(), "RemoveInCollisionGoals goal %zu is in collision removing", i);
    }
  }

  // If all goals have been removed, add the rovers current position as final goal
  if (output_goals_.size() == 0)
  {
    RCLCPP_INFO(node_->get_logger(), "All goals have been removed, doing scuffed solution >:D");
    output_goals_.push_back(current_pose);
  }

  setOutput("output_goals", output_goals_);
  return true;
}

bool RemoveInCollisionGoalsAction::is_goal_in_collision(const PoseStamped & goal)
{
    unsigned int mx = 0;
    unsigned int my = 0;

    if (!global_costmap_->worldToMap(goal.pose.position.x, goal.pose.position.y, mx, my))
    {
      return false;
    }

    GridCell global_cell;
    global_cell.x = static_cast<int>(mx);
    global_cell.y = static_cast<int>(my);
    return !is_cell_free(global_cell);
}

/** Methods from SnapInCollisionGoals */
 
/**
 * @brief Check if a cell is free in both the local and global occupancy grids
 * 
 * @param global_cell A cell with reference to the global occupancy grid
 */
bool RemoveInCollisionGoalsAction::is_cell_free(const GridCell &global_cell)
{
    if (global_cell.x < 0 || global_cell.y < 0) {
      return true;
    }

    const auto global_x = static_cast<unsigned int>(global_cell.x);
    const auto global_y = static_cast<unsigned int>(global_cell.y);
    if (global_x >= global_costmap_->getSizeInCellsX() || global_y >= global_costmap_->getSizeInCellsY()) {
      return true;
    }

    const unsigned char global_cost = global_costmap_->getCost(global_x, global_y);

    double wx = 0.0;
    double wy = 0.0;
    global_costmap_->mapToWorld(global_x, global_y, wx, wy);

    unsigned int local_x = 0;
    unsigned int local_y = 0;
    if (!local_costmap_->worldToMap(wx, wy, local_x, local_y)) {
      return global_cost < cost_threshold_;
    }

    const unsigned char local_cost = local_costmap_->getCost(local_x, local_y);
    RCLCPP_DEBUG(
      node_->get_logger(),
      "Cost at goal cell - global: %u local: %u", global_cost, local_cost);

    return global_cost < cost_threshold_ && local_cost < cost_threshold_;
}

}   // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::RemoveInCollisionGoalsAction>("RemoveInCollisionGoals");
}
