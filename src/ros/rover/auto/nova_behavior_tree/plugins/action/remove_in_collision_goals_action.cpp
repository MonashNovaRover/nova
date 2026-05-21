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
  getInput("snap_last", snap_last_);
  getInput("max_snap_radius", max_snap_radius_);
  getInput("goals_offset", goals_offset_);


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

  // Remove all in collision, and snap last if we need 
  for (size_t i=0; i < input_goals_.size() -1 ; i++)
  {
    Goal goal = input_goals_[i];
    
    if (!is_goal_in_collision(goal))
    {
      output_goals_.push_back(goal);
    }
    else 
    {
      RCLCPP_INFO(node_->get_logger(), "RemoveInCollisionGoals goal %zu is in collision removing", i);
    }
  }

  // Snap the last goal if the flag is set
  if (snap_last_) {
    Goal goal = input_goals_[input_goals.size() - 1];

    if (!is_goal_in_collision(goal))
    {
      output_goals_.push_back(goal);
    }
    else 
    {
      RCLCPP_INFO(node_->get_logger(), "RemoveInCollisionGoals last goal is in collision, snapping to nearest available cell";
      snap(input_goals_[input_goals_.size() - 1], output_goals_);
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

/**
 * @brief Core method of this node. Snaps goals that are in collision to the closest valid position.
 */
bool RemoveInCollisionGoalsAction::snap(Goal goal, Goals output_goals_)
{
  SearchResult result = find_nearest_free_cell(goal.pose.position);

  if (!result.found)
  {
    RCLCPP_WARN(
        node_->get_logger(), "Failed to snap goal (%.2f, %.2f, %.2f) to a free cell",
        goal.pose.position.x, goal.pose.position.y, goal.pose.position.z
    );
    return false;
  }

  if (result.search_radius > 0)
  {
    Point original_pos = goal.pose.position;
    
    // orientTowards() uses worldspace, so x and y are doubles. Hence here we convert the goal x,y from grid to worldspace (unsigned int -> double)
    double wx = 0;
    double wy = 0;
    global_costmap_->mapToWorld(goal.pose.position.x, goal.pose.position.y, wx, wy);
    goal.pose.position.x = static_cast<double>(wx);
    goal.pose.position.y = static_cast<double>(wy);

    // Construct toward point for the origional goal
    Point toward_point{utils::nav2::offsetPose(goal.pose, goals_offset_).position};

    // reorient to corresponding toward point
    utils::nav2::orientTowards(goal.pose, toward_point);

    RCLCPP_INFO(
      node_->get_logger(), "Snapped goal (%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f)",
      original_pos.x, original_pos.y, original_pos.z,
      goal.pose.position.x, goal.pose.position.y, goal.pose.position.z
    );
    RCLCPP_INFO(
      node_->get_logger(), "Original orientation: %d° Snapped orientation: %d°",
      static_cast<int>(std::round(utils::nav2::degrees(tf2::getYaw(input_goals_[input_goals_.size()-1].pose.orientation)))),
      static_cast<int>(std::round(utils::nav2::degrees(tf2::getYaw(goal.pose.orientation))))
    );

    output_goals_.push_back(goal);
    return true;
  }
}

/**
 * @brief Find the nearest free cell using a simple spiral search. For every loop, the search
 * starts from the bottom left corner and goes clockwise.
 * 
 * @param origin The origin point to search around
 */
SearchResult RemoveInCollisionGoalsAction::find_nearest_free_cell(const Point &origin)
{
  // Convert from world to grid 
  unsigned int mx = 0;
  unsigned int my = 0;

  if (!global_costmap_->worldToMap(origin.x, origin.y, mx, my))
  {
    return {{0,0}, false, 0}; // Error SearchResult case, the function calling this will just check the "found" vla
  }

  GridCell global_cell;
  global_cell.x = static_cast<int>(mx);
  global_cell.y = static_cast<int>(my);
  
  // search for the nearest free cell in a spiral pattern
  std::array<int, 2> directions[4] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
  int max_radius = std::ceil(max_snap_radius_ / global_costmap_->getResolution());
  for (int r = 0; r < max_radius; ++r)
  {
      int x = global_cell.x - r;
      int y = global_cell.y - r;
      if (is_area_free({x, y}))
      {
          return {{x, y}, true, r};
      }

      for (int i = 0; i < 4; ++i)
      {
          for (int _ = 0; _ < 2 * r; ++_)
          {
              x += directions[i][0];
              y += directions[i][1];
              if (is_area_free({x, y}))
              {
                  return {{x, y}, true, r};
              }
          }
      }
  }

  return {global_cell, false, max_radius};
}

}   // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::RemoveInCollisionGoalsAction>("RemoveInCollisionGoals");
}
