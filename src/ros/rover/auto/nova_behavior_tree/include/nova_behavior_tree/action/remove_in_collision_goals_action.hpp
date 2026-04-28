// Copyright (c) 2025 Nova Rover
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

#ifndef NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__REMOVE_IN_COLLISION_GOALS_ACTION_HPP_
#define NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__REMOVE_IN_COLLISION_GOALS_ACTION_HPP_

#include <vector>
#include <string>
#include <memory>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/point.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav_msgs/msg/path.hpp"
#include "nav2_util/geometry_utils.hpp"
#include "nav2_util/robot_utils.hpp"
#include "nav2_behavior_tree/bt_utils.hpp"

#include "behaviortree_cpp/action_node.h"
#include "rclcpp/rclcpp.hpp"

namespace nova_behavior_tree
{

using namespace geometry_msgs::msg;
using namespace nav_msgs::msg;

struct GridCell
{
  int x, y;
};

/**
 * @brief A nav2_behavior_tree::BtServiceNode class that removes goals that are in collision in on the global costmap, but only if the rover is within a specified distance.
 * @note It will re-initialize when halted.
 * 
 * @authors Harry Overall
 */
class RemoveInCollisionGoalsAction : public BT::ActionNodeBase
{
public:
  typedef PoseStamped Goal;
  typedef std::vector<Goal> Goals;

  /**
   * @brief A constructor for nav2_behavior_tree::RemoveInCollisionGoals
   * @param service_node_name Service name this node creates a client for
   * @param conf BT node configuration
   */
  RemoveInCollisionGoalsAction(
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

  /**
   * @brief Creates list of BT ports
   * @return BT::PortsList Containing basic ports along with node-specific ports
   */
  static BT::PortsList providedPorts()
  {
    return {
        BT::InputPort<Goals>("input_goals", "Original goals to remove if in collision"),
        BT::InputPort<std::string>("global_frame", "Global reference frame"),
        BT::InputPort<std::string>("robot_base_frame", "robot base frame"),
        BT::InputPort<double>("cost_threshold", 99.0, "Cost threshold for considering a goal in collision"),
        BT::OutputPort<Goals>("output_goals", "Goals with all in collision goals removed"),
      };
  }

private:

  bool is_goal_in_collision(const PoseStamped & goal);
  void wait_for_occu_grids();
  bool remove_goals();
  bool is_cell_free(const GridCell &global_cell);
  bool is_cell_free(const GridCell &cell, const OccupancyGrid::SharedPtr &grid);
  GridCell world_to_grid_cell(const Point &point, const OccupancyGrid::SharedPtr &grid);
  Point grid_cell_to_world(const GridCell &cell, const OccupancyGrid::SharedPtr &grid);

  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<OccupancyGrid>::SharedPtr local_occu_grid_sub_;
  rclcpp::Subscription<OccupancyGrid>::SharedPtr global_occu_grid_sub_;
  OccupancyGrid::SharedPtr local_occu_grid_;
  OccupancyGrid::SharedPtr global_occu_grid_;

  std::string global_frame_, robot_base_frame_;
  std::shared_ptr<tf2_ros::Buffer> tf_;
  double transform_tolerance_;
  double cost_threshold_;
  Goals input_goals_;

  bool initialized_ = false;
  bool set_up_ = false;
};

}  // namespace nova_behavior_tree

#endif  // NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__REMOVE_IN_COLLISION_GOALS_ACTION_HPP_
