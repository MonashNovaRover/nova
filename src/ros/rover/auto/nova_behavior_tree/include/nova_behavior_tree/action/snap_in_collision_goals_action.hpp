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

/**
 * @brief Action node for snapping goals that are in collision to the closest valid position.
 * To ensure correct orientation, 'toward points' are used to determine the orientation of the
 * snapped goal. Toward points are points to which the original goals were pointed towards.
 * 
 * To find a suitable pose to snap to, a spiral search pattern is used to find the nearest free
 * or unknown cell in the occupancy grid.
 * 
 * @authors Terry Tian
 */

#ifndef NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__SNAP_IN_COLLISION_GOALS_ACTION_HPP_
#define NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__SNAP_IN_COLLISION_GOALS_ACTION_HPP_

#include <vector>
#include <string>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/point.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "behaviortree_cpp/action_node.h"

namespace nova_behavior_tree
{

using namespace geometry_msgs::msg;
using namespace nav_msgs::msg;

struct GridCell
{
  int x, y;
};

struct SearchResult
{
  GridCell cell;
  bool found;
  int search_radius;
};

struct TowardPoint
{
  Point point; // The point to which the original goal was pointed towards
  PoseStamped goal; // The original goal
};

class SnapInCollisionGoalsAction : public BT::ActionNodeBase
{
public:
  typedef PoseStamped Goal;
  typedef std::vector<Goal> Goals;

  SnapInCollisionGoalsAction(
    const std::string & xml_tag_name,
    const BT::NodeConfiguration & conf);

  /**
   * @brief Function to initialize variables,
   * called only once in the lifecycle of the navigator
   */
  void initialize();

  /**
   * @brief Function to setup variables, called every time
   * navigation is started
   */
  void setup();

  /**
   * @brief The main override required by a BT node
   * @return BT::NodeStatus Status of tick execution
   */
  BT::NodeStatus tick() override;
  
  /**
   * @brief Function called when the BT is halted,
   * use to cleanup or reset state
   */
  void halt() override;

  static BT::PortsList providedPorts()
  {
    return {
      BT::InputPort<double>("goals_offset" "Approximate distance goals are offset"),
      BT::InputPort<double>("max_snap_radius", 5.0, "Maximum radius (m) to snap goals to"),
      BT::InputPort<Goals>("input_goals", "Original goals to snap if in collision"),
      BT::OutputPort<Goals>("output_goals", "Goals with all in collision goals snapped"),
    };
  }

private:
  void wait_for_occu_grids();
  void update_toward_points();
  bool snap_goals();
  SearchResult find_nearest_free_cell(const Point &origin);
  bool is_area_free(const GridCell &center);
  bool is_cell_free(const GridCell &global_cell);
  bool is_cell_free(const GridCell &cell, const OccupancyGrid::SharedPtr &grid);
  GridCell world_to_grid_cell(const Point &point, const OccupancyGrid::SharedPtr &grid);
  Point grid_cell_to_world(const GridCell &cell, const OccupancyGrid::SharedPtr &grid);

  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<OccupancyGrid>::SharedPtr local_occu_grid_sub_;
  rclcpp::Subscription<OccupancyGrid>::SharedPtr global_occu_grid_sub_;
  OccupancyGrid::SharedPtr local_occu_grid_;
  OccupancyGrid::SharedPtr global_occu_grid_;
  
  double goals_offset_;
  std::vector<TowardPoint> toward_points_;
  double max_snap_radius_;
  double footprint_radius_;
  Goals input_goals_;
  
  bool initialized_ = false;
  bool set_up_ = false;
};

}  // namespace nova_behavior_tree

#endif  // NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__SNAP_IN_COLLISION_GOALS_ACTION_HPP_