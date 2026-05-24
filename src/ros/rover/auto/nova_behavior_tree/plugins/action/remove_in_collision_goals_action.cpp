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

  // We need a way of getting the size of the local costmap here

  // Subscribe to local and global costmaps via Nav2 costmap transport.
  local_costmap_sub_ = std::make_unique<nav2_costmap_2d::CostmapSubscriber>(
    node_, "/local_costmap/costmap_raw");

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
        "RemoveInCollisionGoals waiting for local costmap.");
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
  }
  catch (const std::runtime_error &)
  {
    local_costmap_.reset();
    return false;
  }
  return static_cast<bool>(local_costmap_);
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
  size_t remove_goals_end_index = input_goals_.size();
  if (snap_last_) remove_goals_end_index = remove_goals_end_index - 1; // ignore last in this case

  // Remove all in collision, and snap last if we need 
  for (size_t i=0; i < remove_goals_end_index ; i++)
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
    Goal goal = input_goals_[input_goals_.size() - 1];

    if (!is_goal_in_collision(goal))
    {
      output_goals_.push_back(goal);
    }
    else 
    {
      RCLCPP_INFO(node_->get_logger(), "RemoveInCollisionGoals last goal is in collision, snapping to nearest available cell");
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

  // If goal is outside bounds of local costmap, assume not in collision
  double wx = 0.0;
  double wy = 0.0;
  local_costmap_->mapToWorld(goal.pose.position.x, goal.pose.position.y, wx, wy);

  // Transform goal coords from map -> odom frame
  tf_->transform(goal, goal_in_odom_, "odom",tf2::TimePointZero);

  // Convert from worldspace to gridspace
  unsigned int mx, my;
  if (!local_costmap_->worldToMap(goal_in_odom_.pose.position.x, goal_in_odom_.pose.position.y, mx, my))
  {
    // Point falls outside of the grid, assume not in collision
    return false;
  }

  // Check costmap value at this point
  GridCell grid_cell;
  grid_cell.x = static_cast<int>(mx);
  grid_cell.y = static_cast<int>(my);
  return !is_cell_free(grid_cell);

  // // If we are ignoring global, just do this simple check
  // if (ignore_global_costmap_)
  // {
  //   unsigned int local_x = 0;
  //   unsigned int local_y = 0;
  //   if (!local_costmap_->worldToMap(goal.pose.position.x, goal.pose.position.y, local_x, local_y)) {
  //     return false;
  //   }

  //   GridCell local_cell;
  //   local_cell.x = static_cast<int>(local_x);
  //   local_cell.y = static_cast<int>(local_y);
  //   return !is_cell_free(local_cell);
  // } 
  
  // // Otherwise check both maps
  // else
  // {
  //   unsigned int mx = 0;
  //   unsigned int my = 0;

  //   if (!global_costmap_->worldToMap(goal.pose.position.x, goal.pose.position.y, mx, my))
  //   {
  //     return false;
  //   }

  //   GridCell global_cell;
  //   global_cell.x = static_cast<int>(mx);
  //   global_cell.y = static_cast<int>(my);
  //   return !is_cell_free(global_cell);
  // }
}

/** Methods from SnapInCollisionGoals */
 
/**
 * @brief Check if a cell is free in both the local and global occupancy grids
 * 
 * @param grid_cell A cell with reference to the costmap grid
 */
bool RemoveInCollisionGoalsAction::is_cell_free(const GridCell &grid_cell)
{
    const unsigned char local_cost = local_costmap_->getCost(grid_cell.x, grid_cell.y);
    return local_cost < cost_threshold_;

//     const unsigned char local_cost = local_costmap_->getCost(grid_cell.x, grid_cell.y);
//     return !(local_cost < cost_threshold_);


//     // If we are using local costmap only
//     if (ignore_global_costmap_)
//     {
//       const unsigned char local_cost = local_costmap_->getCost(grid_cell.x, grid_cell.y);
//       return !(local_cost < cost_threshold_);
//     } 
    
//     // Otherwise check both
//     else
//     {
//       const auto global_x = static_cast<unsigned int>(grid_cell.x);
//       const auto global_y = static_cast<unsigned int>(grid_cell.y);
      
//       if (global_x >= global_costmap_->getSizeInCellsX() || global_y >= global_costmap_->getSizeInCellsY()) {
//         return true;
//       }

//       const unsigned char global_cost = global_costmap_->getCost(global_x, global_y);

//       double wx = 0.0;
//       double wy = 0.0;
//       global_costmap_->mapToWorld(global_x, global_y, wx, wy);

//       unsigned int local_x = 0;
//       unsigned int local_y = 0;
//       if (!local_costmap_->worldToMap(wx, wy, local_x, local_y)) {
//         return global_cost < cost_threshold_;
//       }

//       const unsigned char local_cost = local_costmap_->getCost(local_x, local_y);
//       RCLCPP_DEBUG(
//         node_->get_logger(),
//         "Cost at goal cell - global: %u local: %u", global_cost, local_cost);

//       return global_cost < cost_threshold_ && local_cost < cost_threshold_;
//     }
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
    
    double wx = 0;
    double wy = 0;
    local_costmap_->mapToWorld(goal.pose.position.x, goal.pose.position.y, wx, wy);
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
  }

  RCLCPP_INFO(node_->get_logger(), "Pushing back");
  output_goals_.push_back(goal);
  return true;
}

/**
 * @brief Find the nearest free cell using a simple spiral search. For every loop, the search
 * starts from the bottom left corner and goes clockwise.
 * 
 * @param origin The origin point to search around
 */
SearchResult RemoveInCollisionGoalsAction::find_nearest_free_cell(const Point &origin)
{
  // Convert from worldspace to gridspace
  unsigned int mx = 0;
  unsigned int my = 0;

  if (!local_costmap_->worldToMap(origin.x, origin.y, mx, my))
  {
    RCLCPP_INFO(node_->get_logger(), "Goal out of bounds");
    return {{0,0}, false, 0};
  }

  GridCell local_cell;
  local_cell.x = static_cast<int>(mx);
  local_cell.y = static_cast<int>(my);
  
  // search for the nearest free cell in a spiral pattern
  std::array<int, 2> directions[4] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
  int max_radius = std::ceil(max_snap_radius_ / local_costmap_->getResolution());

  for (int r = 0; r < max_radius; ++r)
  {
      int x = local_cell.x - r;
      int y = local_cell.y - r;
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
                  RCLCPP_INFO(node_->get_logger(), "Found nearest free cell");
                  return {{x, y}, true, r};
              }
          }
      }
  }
  RCLCPP_INFO(node_->get_logger(), "Unable to find cell");
  return {local_cell, false, max_radius};
}

/**
 * @brief Check if the area around the center cell is free
 * To actually check the area of a circle instead of a square, we mark a circle border as 'visited'
 * using the midpoint circle algorithm https://www.youtube.com/watch?v=hpiILbMkF9w&ab_channel=NoBSCode
 * so we can then run BFS from the center cell to check if the area is free.
 * 
 * The midpoint circle algorithm is very hard to understand by just looking at the code, so either
 * watch the video or just accept that it works.
 * 
 * Note: the variant of the algorithm in the video starts drawing from (0, -r) because that is
 * the top of the circle in screen space. I have modified it to start from (0, r), as we are
 * not in screen space.
 * 
 * @param center The center cell of the area to check in the global occupancy grid
 */
bool RemoveInCollisionGoalsAction::is_area_free(const GridCell &center)
{

  // Get robot radius
  !node_->get_parameter_or("robot_radius", footprint_radius_, 0.85);
  // if (!node_->get_parameter_or("robot_radius", footprint_radius_, 0.85))
  // {
  //   RCLCPP_ERROR(node_->get_logger(), "SnapInCollisionGoals Failed to get local footprint, using default value of 0.85m");
  // }

  // avoid extra computation if center cell is not free
  if (!is_cell_free(center))
  {
      return false;
  }
  
  int radius = std::ceil(footprint_radius_ / local_costmap_->getResolution());
  int side = 2*radius + 1;
  std::vector<bool> visited(side * side, false);
  auto mark_visited = [&](int x, int y)
  {
      int index = (y + radius) * side + (x + radius);
      visited[index] = true;
  };
  auto is_visited = [&](int x, int y) -> bool
  {
      int index = (y + radius) * side + (x + radius);
      return visited[index];
  };
  auto rel_to_abs = [&](int x, int y) -> GridCell
  {
      return {center.x + x, center.y + y};
  };
  
  // mark circle boundary as visited
  // midpoint circle algorithm
  std::array<int, 2> quadrants[4] = {{1, 1}, {-1, 1}, {-1, -1}, {1, -1}};
  int x = 0, y = radius, p = -radius;
  while (x < y)
  {
      if (p > 0)
      {
          y -= 1;
          p += 2*(x-y) + 1;
      }
      else
      {
          p += 2*x + 1;
      }

      for (const auto &q : quadrants)
      {
          int dx = q[0] * x, dy = q[1] * y;

          if (!is_cell_free(rel_to_abs(dx, dy)) || !is_cell_free(rel_to_abs(dy, dx)))
          {
              return false;
          }

          mark_visited(dx, dy);
          mark_visited(dy, dx);
      }
  }

  // BFS from center
  std::array<int, 2> directions[4] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
  std::queue<GridCell> q;
  mark_visited(0, 0);
  q.push({0, 0});
  while (!q.empty())
  {
      GridCell curr = q.front();
      q.pop();

      for (const auto &d : directions)
      {
          int nx = curr.x + d[0], ny = curr.y + d[1];
          if (nx < -radius || nx > radius || ny < -radius || ny > radius)
          {
              continue;
          }

          if (!is_visited(nx, ny))
          {
              if (!is_cell_free(rel_to_abs(nx, ny)))
              {
                  return false;
              }
              mark_visited(nx, ny);
              q.push({nx, ny});
          }
      }
  }

  return true;
}

}   // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::RemoveInCollisionGoalsAction>("RemoveInCollisionGoals");
}
