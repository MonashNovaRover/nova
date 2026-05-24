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
#include "nav_msgs/msg/path.hpp"
#include "nav2_costmap_2d/costmap_2d.hpp"
#include "nav2_costmap_2d/costmap_subscriber.hpp"
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
        BT::InputPort<std::string>("global_frame", "map", "Global reference frame"),
        BT::InputPort<std::string>("robot_base_frame", "base_link", "Robot base frame"),

        // SnapInCollisionGoals stuff
        BT::InputPort<bool>("snap_last", "If the last goal should never be removed and instead snapped"),
        BT::InputPort<double>("max_snap_radius", 5.0, "Maximum radius (m) to snap goals to"),
        BT::InputPort<double>("goals_offset", 1.5, "Approximate distance of offset when calculating toward point"),

        // RemoveInCollisionGoals ports
        // 255 = unknown, 254 = lethal, 253 = inscribed
        BT::InputPort<double>("cost_threshold", 253.0, "Cost threshold for considering a goal in collision (exclusive)"),
        BT::OutputPort<Goals>("output_goals", "Goals with all in collision goals removed"),

      };
  }

private:

  bool is_goal_in_collision(Goal goal);
  bool remove_goals();
  bool have_costmaps();
  bool snap(Goal goal, Goals &output_goals_);
  bool is_cell_free(const GridCell &global_cell);
  bool is_area_free(const GridCell &center);
  SearchResult find_nearest_free_cell(Goal goal);

  rclcpp::Node::SharedPtr node_;
  std::unique_ptr<nav2_costmap_2d::CostmapSubscriber> local_costmap_sub_;
  std::unique_ptr<nav2_costmap_2d::CostmapSubscriber> global_costmap_sub_;
  std::shared_ptr<nav2_costmap_2d::Costmap2D> local_costmap_;
  std::shared_ptr<nav2_costmap_2d::Costmap2D> global_costmap_;

  std::string global_frame_, robot_base_frame_;
  geometry_msgs::msg::PoseStamped goal_in_odom_, goal_in_map_;
  std::shared_ptr<tf2_ros::Buffer> tf_;
  Goals input_goals_;
  double transform_tolerance_;
  double cost_threshold_;

  bool snap_last_;
  double max_snap_radius_;
  double goals_offset_;
  double footprint_radius_;

  bool initialized_ = false;
  bool set_up_ = false;

};

}  // namespace nova_behavior_tree

#endif  // NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__REMOVE_IN_COLLISION_GOALS_ACTION_HPP_
