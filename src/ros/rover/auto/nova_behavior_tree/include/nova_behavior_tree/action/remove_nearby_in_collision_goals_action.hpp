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
 * @brief Action node for removing nearby goals that are in collision
 * Only goals within a certain distance of the rover will be considered for removal, all goals outside of the
 * specified distance will be kept. This is to avoid removing goals inside obstacles that are very far away, as
 * we may not yet have the most accurate data about that distant obstacle.
 * 
 * @authors Harry Overall
 */

#ifndef NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__REMOVE_NEARBY_IN_COLLISION_GOALS_ACTION_HPP_
#define NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__REMOVE_NEARBY_IN_COLLISION_GOALS_ACTION_HPP_

#include <vector>
#include <string>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/point.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
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
class RemoveNearbyInCollisionGoalsAction : public BT::ActionNodeBase
{
public:
  typedef PoseStamped Goal;
  typedef std::vector<Goal> Goals;

  /**
   * @brief A constructor for nav2_behavior_tree::RemoveInCollisionGoals
   * @param service_node_name Service name this node creates a client for
   * @param conf BT node configuration
   */
  RemoveNearbyInCollisionGoalsAction(
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
        BT::InputPort<double>("max_distance_threshold", 5.0, "Maximum radius (m) for a goal to be considered for removal"),
        BT::InputPort<Goals>("input_goals", "Original goals to remove if in collision"),
        BT::InputPort<geometry_msgs::msg::PoseStamped>("current_pose", "Current pose input"),
        BT::OutputPort<Goals>("output_goals", "Goals with all in collision goals removed"),
      };
  }

private:

  double euclidean_distance(const PoseStamped & a, const PoseStamped & b);
  bool is_goal_in_collision(const PoseStamped & goal);
  void wait_for_occu_grids();
  bool remove_goals();
  bool is_cell_free(const GridCell &global_cell);
  bool is_cell_free(const GridCell &cell, const OccupancyGrid::SharedPtr &grid);
  GridCell world_to_grid_cell(const Point &point, const OccupancyGrid::SharedPtr &grid);
  Point grid_cell_to_world(const GridCell &cell, const OccupancyGrid::SharedPtr &grid);

  rclcpp::Node::SharedPtr node_;
  std::string global_frame_, robot_base_frame_;
  std::shared_ptr<tf2_ros::Buffer> tf_;
  rclcpp::Subscription<OccupancyGrid>::SharedPtr local_occu_grid_sub_;
  rclcpp::Subscription<OccupancyGrid>::SharedPtr global_occu_grid_sub_;
  OccupancyGrid::SharedPtr local_occu_grid_;
  OccupancyGrid::SharedPtr global_occu_grid_;
  double transform_tolerance_;
  
  Goals input_goals_;
  double max_distance_threshold_;

  bool initialized_ = false;
  bool set_up_ = false;
};

}  // namespace nova_behavior_tree

#endif  // NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__REMOVE_NEARBY_IN_COLLISION_GOALS_ACTION_HPP_
