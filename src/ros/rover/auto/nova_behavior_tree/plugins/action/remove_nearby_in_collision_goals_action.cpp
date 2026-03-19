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

#include <string>
#include <vector>
#include <array>
#include <cmath>
#include <queue>
#include <chrono>
#include <thread>

#include "nav2_util/geometry_utils.hpp"
#include "rclcpp/logging.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/point.hpp"
#include "tf2/utils.h"
#include "nav2_behavior_tree/bt_utils.hpp"
#include "nav_msgs/msg/path.hpp"
#include "behaviortree_cpp/decorator_node.h"

#include "nova_behavior_tree/action/remove_nearby_in_collision_goals_action.hpp"
#include "nova_behavior_tree/nav2_utils.hpp"
#include "rclcpp/rclcpp.hpp"

namespace nova_behavior_tree
{

using namespace nav2_util::geometry_utils;
using namespace geometry_msgs::msg;
using namespace nav_msgs::msg;

RemoveNearbyInCollisionGoalsAction::RemoveNearbyInCollisionGoalsAction(
  const std::string & name,
  const BT::NodeConfiguration & conf)
: BT::ActionNodeBase(name, conf)
{
  node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
  tf_ = config().blackboard->get<std::shared_ptr<tf2_ros::Buffer>>("tf_buffer");
  node_->get_parameter("transform_tolerance", transform_tolerance_);
  global_frame_ = BT::deconflictPortAndParamFrame<std::string>(
    node, "global_frame", this);
  robot_base_frame_ = BT::deconflictPortAndParamFrame<std::string>(
    node, "robot_base_frame", this);
}

void RemoveNearbyInCollisionGoalsAction::initialize()
{
  // Subscribe to local and global costmaps' occupancy grids
  local_occu_grid_sub_ = node_->create_subscription<OccupancyGrid>(
        "/local_costmap/costmap", 1,
        [this](const OccupancyGrid::SharedPtr msg) -> void
        {
          local_occu_grid_ = msg;
          RCLCPP_DEBUG(node_->get_logger(), "Received local costmap");
      }
  );
  global_occu_grid_sub_ = node_->create_subscription<OccupancyGrid>(
      "/global_costmap/costmap", 1,
      [this](const OccupancyGrid::SharedPtr msg) -> void
      {
          global_occu_grid_ = msg;
          RCLCPP_DEBUG(node_->get_logger(), "Received global costmap");
      }
  );

  wait_for_occu_grids();
  RCLCPP_INFO(node_->get_logger(), "RemoveNearbyInCollisionGoals successfully initialized!");
  initialized_ = true;
}

void RemoveNearbyInCollisionGoalsAction::setup()
{
  if (!initialized_)
  {
    initialize();
  }
  
  if (!local_occu_grid_ || !global_occu_grid_)
  {
    wait_for_occu_grids();
  }

  RCLCPP_INFO(node_->get_logger(), "RemoveNearbyInCollisionGoals successfully set up!");
        
  set_up_ = true;
}

void RemoveNearbyInCollisionGoalsAction::halt()
{
  set_up_ = false;
}

inline BT::NodeStatus RemoveNearbyInCollisionGoalsAction::tick()
{
  if (!set_up_)
  {
      setup();
  }

  // at this point, we should already have the occupancy grids
  // however, this is just a safeguard
  if (!local_occu_grid_ || !global_occu_grid_)
  {
      wait_for_occu_grids();
  }
  
  getInput("input_goals", input_goals_);

  // this is necessary to receive updates on the occupancy grids
  rclcpp::spin_some(node_);

  // remove goals
  if (remove_goals())
  {
      return BT::NodeStatus::SUCCESS;
  }

  // On failure, just return input goals
  RCLCPP_ERROR(node_->get_logger(), "RemoveNearbyInCollisionGoals Failed remove nearby goals, goals remain unchanged.");
  setOutput("output_goals", input_goals_);
  return BT::NodeStatus::FAILURE;
}

void RemoveNearbyInCollisionGoalsAction::wait_for_occu_grids()
{
    // measure time to initialize
    auto start = std::chrono::high_resolution_clock::now();
    while (!local_occu_grid_ || !global_occu_grid_)
    {
        rclcpp::spin_some(node_);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    auto end = std::chrono::high_resolution_clock::now();
    RCLCPP_INFO(
        node_->get_logger(), "RemoveNearbyInCollisionGoals waited %.2fms for occupancy grids",
        std::chrono::duration<double, std::milli>(end - start).count()
    );
}

/**
 * @brief Returns the 2D Euclidean distance between two poses.
 */
double RemoveNearbyInCollisionGoalsAction::euclidean_distance(
    const PoseStamped & a,
    const PoseStamped & b)
{
    const double dx = a.pose.position.x - b.pose.position.x;
    const double dy = a.pose.position.y - b.pose.position.y;
    return std::sqrt(dx * dx + dy * dy);
}

bool RemoveNearbyInCollisionGoalsAction::is_goal_in_collision(const PoseStamped & goal)
{
    GridCell global_cell = world_to_grid_cell(goal.pose.position, global_occu_grid_);
    return !is_cell_free(global_cell);
}

bool RemoveNearbyInCollisionGoalsAction::remove_goals()
{
  geometry_msgs::msg::PoseStamped current_pose_;

  if (!nav2_util::getCurrentPose(
      current_pose_, *tf_, global_frame_, robot_base_frame_, transform_tolerance_))
  {
    RCLCPP_WARN(config().blackboard->get<rclcpp::Node::SharedPtr>("node")->get_logger(),
      "Current robot pose is not available.");
    return false;
  }

  Goals output_goals_;
  for (size_t i=0; i < input_goals_.size(); i++)
  {
    Goal goal = input_goals_[i];

    // Keep goal if it is outside max distance to consider for removal
    const double dist = euclidean_distance(current_pose_, goal);
    if (dist > max_distance_threshold_)
    {
      output_goals_.push_back(goal);
    } 

    // Otherwise remove goal if it is inside an obstacle
    else 
    {
      if (!is_goal_in_collision(goal))
      {
        output_goals_.push_back(goal);
      }
      else 
      {
        RCLCPP_INFO(node_->get_logger(), "RemoveNearbyInCollisionGoals goal %zu is within threshold and in collision, removing", i);
      }
    }
  }
  setOutput("output_goals", output_goals_);
  return true;
}

/** Methods from SnapInCollisionGoals */
 
/**
 * @brief Check if a cell is free in both the local and global occupancy grids
 * 
 * @param global_cell A cell with reference to the global occupancy grid
 */
bool RemoveNearbyInCollisionGoalsAction::is_cell_free(const GridCell &global_cell)
{
    GridCell local_cell = world_to_grid_cell(grid_cell_to_world(global_cell, global_occu_grid_), local_occu_grid_);
    return is_cell_free(global_cell, global_occu_grid_) && is_cell_free(local_cell, local_occu_grid_);
}

/**
 * @brief Check if a cell is free in a given occupancy grid
 * 
 * @param cell A cell with reference to the occupancy grid
 * @param grid The occupancy grid to check
 */
bool RemoveNearbyInCollisionGoalsAction::is_cell_free(const GridCell &cell, const OccupancyGrid::SharedPtr &grid)
{
    if (cell.x < 0 || cell.x >= static_cast<int>((*grid).info.width) ||
        cell.y < 0 || cell.y >= static_cast<int>((*grid).info.height))
    {
        return true; // treat out-of-bounds cells as free
    }
    int index = cell.y * (*grid).info.width + cell.x;
    return (*grid).data[index] <= 0;
}

/**
 * @brief Convert a world point to a grid cell in the specified occupancy grid
 * 
 * @param point The point to convert
 * @param grid The occupancy grid to use for conversion
 */
GridCell RemoveNearbyInCollisionGoalsAction::world_to_grid_cell(const Point &point, const OccupancyGrid::SharedPtr &grid)
{
    GridCell cell;
    cell.x = static_cast<int>(std::round((point.x - (*grid).info.origin.position.x) / (*grid).info.resolution));
    cell.y = static_cast<int>(std::round((point.y - (*grid).info.origin.position.y) / (*grid).info.resolution));
    return cell;
}

/**
 * @brief Convert a grid cell in the specified occupancy grid to a world point
 * 
 * @param cell The cell to convert
 * @param grid The occupancy grid the cell is in
 */
Point RemoveNearbyInCollisionGoalsAction::grid_cell_to_world(const GridCell &cell, const OccupancyGrid::SharedPtr &grid)
{
    Point point;
    point.x = (*grid).info.origin.position.x + (cell.x * (*grid).info.resolution);
    point.y = (*grid).info.origin.position.y + (cell.y * (*grid).info.resolution);
    return point;
}

}   // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::RemoveNearbyInCollisionGoalsAction>("RemoveNearbyInCollisionGoals");
}
