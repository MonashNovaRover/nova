// Copyright (c) 2021 Samsung Research America
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

#include <string>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <thread>

#include "nav2_util/geometry_utils.hpp"
#include "rclcpp/logging.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/point.hpp"
#include "tf2/LinearMath/Vector3.h"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "tf2/utils.h"

#include "nova_behavior_tree/snap_in_collision_goals_action.hpp"
#include "nova_behavior_tree/nav2_utils.hpp"

namespace nova_behavior_tree
{

    using namespace nav2_util::geometry_utils;
    using namespace nav_msgs::msg;

    SnapInCollisionGoalsAction::SnapInCollisionGoalsAction(
    const std::string & name,
    const BT::NodeConfiguration & conf)
    : BT::ActionNodeBase(name, conf)
    {
    }

    void SnapInCollisionGoalsAction::initialize()
    {
        node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");

        getInput("max_snap_radius", max_snap_radius_);
        getInput("update_radius", update_radius_);
        
        double initial_goals_offset;
        getInput("initial_goals_offset", initial_goals_offset);
        getInput("input_goals", input_goals_);

        // calculate toward points for intial goals
        for (const auto &goal : input_goals_)
        {
            geometry_msgs::msg::Point p = goal.pose.position;
            
            tf2::Vector3 v(p.x, p.y, p.z);
            v += v.normalized() * initial_goals_offset;

            geometry_msgs::msg::Point toward_point;
            tf2::toMsg(v, toward_point);

            toward_points_.push_back(toward_point);
        }

        // subscribe to local and global costmaps' occupancy grids
        local_occu_grid_sub_ = node_->create_subscription<OccupancyGrid>(
            "/local_costmap/costmap", 1,
            [this](const OccupancyGrid::SharedPtr msg) -> void
            {
                RCLCPP_INFO(node_->get_logger(), "Received local costmap");
                local_occu_grid_ = msg;
            }
        );
        global_occu_grid_sub_ = node_->create_subscription<OccupancyGrid>(
            "/global_costmap/costmap", 1,
            [this](const OccupancyGrid::SharedPtr msg) -> void
            {
                RCLCPP_INFO(node_->get_logger(), "Received global costmap");
                global_occu_grid_ = msg;
            }
        );

        // measure time to initialize
        auto start = std::chrono::high_resolution_clock::now();
        while (!local_occu_grid_ || !global_occu_grid_)
        {
            rclcpp::spin_some(node_);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        auto end = std::chrono::high_resolution_clock::now();
        RCLCPP_INFO(
            node_->get_logger(), "SnapInCollisionGoalsAction waited %.2fms for occupancy grids",
            std::chrono::duration<double, std::milli>(end - start).count()
        );

        RCLCPP_INFO(node_->get_logger(), "SnapInCollisionGoalsAction successfully initialized");
        
        initialized_ = true;
    }

    inline BT::NodeStatus SnapInCollisionGoalsAction::tick()
    {
        if (!initialized_)
        {
            initialize();
        }

        if (!local_occu_grid_)
        {
            throw std::runtime_error("Local occupancy grid is not available");
        }
        if (!global_occu_grid_)
        {
            throw std::runtime_error("Global occupancy grid is not available");
        }
        
        getInput("cube_goal_entries", cube_goal_entries_);
        getInput("input_goals", input_goals_);

        // update toward points if goals have been removed
        while (toward_points_.size() > input_goals_.size())
        {
            toward_points_.erase(toward_points_.begin());
        }
        
        // update toward points with cube goals
        std::sort(cube_goal_entries_.begin(), cube_goal_entries_.end(),
            [](const GoalEntry &a, const GoalEntry &b) -> bool
            {
                return a.index < b.index;
            }
        );

        for (const auto &entry : cube_goal_entries_)
        {
            if (entry.index > toward_points_.size())
            {
                RCLCPP_ERROR(
                    node_->get_logger(), "Cube goal index (%d) exceeds toward_points_ size (%lu)",
                    entry.index, toward_points_.size()
                );
                continue;
            }

            if (entry.index == toward_points_.size())
            {
                toward_points_.push_back(entry.pose.position);
                continue;
            }

            // update or insert?
            if (euclidean_distance(entry.pose.position, toward_points_[entry.index]) < update_radius_)
            {
                toward_points_[entry.index] = entry.pose.position;
            }
            else
            {
                toward_points_.insert(toward_points_.begin() + entry.index, entry.pose.position);
            }
        }

        // snap!
        if (snap_goals())
        {
            return BT::NodeStatus::SUCCESS;
        }

        return BT::NodeStatus::FAILURE;
    }

    bool SnapInCollisionGoalsAction::snap_goals()
    {
        unsigned int failed_snaps = 0;
        Goals output_goals_;

        for (size_t i = 0; i < input_goals_.size(); ++i)
        {
            Goal goal = input_goals_[i];
            Point2D goal_point_2d = {goal.pose.position.x, goal.pose.position.y};
            SearchResult result = find_nearest_free_cell(goal_point_2d);

            if (!result.found)
            {
                failed_snaps += 1;
                RCLCPP_WARN(
                    node_->get_logger(), "Failed to snap goal (%.2f, %.2f, %.2f) to a free cell",
                    goal.pose.position.x, goal.pose.position.y, goal.pose.position.z
                );
                continue;
            }
            
            goal.pose.position.x = grid_cell_to_world(result.cell, global_occu_grid_).x;
            goal.pose.position.y = grid_cell_to_world(result.cell, global_occu_grid_).y;

            // TODO: refactor this into a function in nova_behavior_tree/nav2_utils.hpp
            // reorient to corresponding toward point
            tf2::Vector3 v1;
            tf2::Vector3 v2;
            tf2::fromMsg(goal.pose.position, v1);
            tf2::fromMsg(toward_points_[i], v2);

            tf2::Vector3 direction_normal = (v2 - v1).normalized();
            tf2::Vector3 ref_axis(1.0, 0.0, 0.0);
            tf2::Vector3 axis = ref_axis.cross(direction_normal);
            tf2::Quaternion goal_orientation;
            goal_orientation.setRotation(axis, std::acos(ref_axis.dot(direction_normal)));

            goal.pose.orientation = tf2::toMsg(goal_orientation);
            output_goals_.push_back(goal);

            geometry_msgs::msg::Point original_point;
            original_point.x = goal_point_2d.x;
            original_point.y = goal_point_2d.y;
            original_point.z = goal.pose.position.z;
            if (euclidean_distance(original_point, goal.pose.position) > 0.01)
            {
                RCLCPP_INFO(
                    node_->get_logger(), "Snapped goal (%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f)",
                    goal_point_2d.x, goal_point_2d.y, goal.pose.position.z,
                    goal.pose.position.x, goal.pose.position.y, goal.pose.position.z
                );
                RCLCPP_INFO(
                    node_->get_logger(), "Original orientation: %d° Snapped orientation: %d°",
                    static_cast<int>(std::round(utils::nav2::degrees(tf2::getYaw(input_goals_[i].pose.orientation)))),
                    static_cast<int>(std::round(utils::nav2::degrees(tf2::getYaw(goal.pose.orientation))))
                );
            }
        }

        setOutput("output_goals", output_goals_);

        if (failed_snaps > 0)
        {
            RCLCPP_WARN(node_->get_logger(), "Failed to snap %u goals", failed_snaps);
        }
        
        return failed_snaps == 0;
    }

    SearchResult SnapInCollisionGoalsAction::find_nearest_free_cell(const Point2D &origin)
    {
        GridCell global_cell = world_to_grid_cell(origin, global_occu_grid_);

        // search for the nearest free cell in a spiral pattern
        std::array<int, 2> directions[4] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
        int max_radius = std::ceil(max_snap_radius_ / (*global_occu_grid_).info.resolution);
        for (int r = 0; r < max_radius; ++r)
        {
            int x = global_cell.x - r;
            int y = global_cell.y - r;
            if (is_cell_free({x, y}))
            {
                return {{x, y}, true};
            }

            for (int i = 0; i < 4; ++i)
            {
                for (int _ = 0; _ < 2 * r; ++_)
                {
                    x += directions[i][0];
                    y += directions[i][1];
                    if (is_cell_free({x, y}))
                    {
                        return {{x, y}, true};
                    }
                }
            }
        }

        return {global_cell, false};
    }

    bool SnapInCollisionGoalsAction::is_cell_free(const GridCell &global_cell)
    {
        GridCell local_cell = world_to_grid_cell(grid_cell_to_world(global_cell, global_occu_grid_), local_occu_grid_);
        return is_cell_free(global_cell, global_occu_grid_) && is_cell_free(local_cell, local_occu_grid_);
    }

    bool SnapInCollisionGoalsAction::is_cell_free(const GridCell &cell, const OccupancyGrid::SharedPtr &grid)
    {
        if (cell.x < 0 || cell.x >= static_cast<int>((*grid).info.width) ||
            cell.y < 0 || cell.y >= static_cast<int>((*grid).info.height))
        {
            return true; // treat out-of-bound (unknown) cells as free
        }
        int index = cell.y * (*grid).info.width + cell.x;
        return (*grid).data[index] <= 0;
    }

    GridCell SnapInCollisionGoalsAction::world_to_grid_cell(const Point2D &point, const OccupancyGrid::SharedPtr &grid)
    {
        GridCell cell;
        cell.x = (point.x - (*grid).info.origin.position.x) / (*grid).info.resolution;
        cell.y = (point.y - (*grid).info.origin.position.y) / (*grid).info.resolution;
        return cell;
    }

    Point2D SnapInCollisionGoalsAction::grid_cell_to_world(const GridCell &cell, const OccupancyGrid::SharedPtr &grid)
    {
        // 0.5 is added to use the center of the cell, rather than the corner
        Point2D point;
        point.x = (*grid).info.origin.position.x + (cell.x + 0.5) * (*grid).info.resolution;
        point.y = (*grid).info.origin.position.y + (cell.y + 0.5) * (*grid).info.resolution;
        return point;
    }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::SnapInCollisionGoalsAction>("SnapInCollisionGoals");
}