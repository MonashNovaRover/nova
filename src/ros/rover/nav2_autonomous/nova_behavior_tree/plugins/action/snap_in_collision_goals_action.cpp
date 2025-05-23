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
#include <queue>
#include <chrono>
#include <thread>

#include "nav2_util/geometry_utils.hpp"
#include "rclcpp/logging.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/point.hpp"
#include "tf2/utils.h"
#include "nav2_behavior_tree/bt_utils.hpp"

#include "nova_behavior_tree/action/snap_in_collision_goals_action.hpp"
#include "nova_behavior_tree/nav2_utils.hpp"

namespace nova_behavior_tree
{

    using namespace nav2_util::geometry_utils;
    using namespace geometry_msgs::msg;
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
        goals_offset_ = BT::deconflictPortAndParamFrame<double>(node_, "goal_offset_radius", this);

        // subscribe to local and global costmaps' occupancy grids
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

        // get footprint radius
        if (!node_->get_parameter_or("robot_radius", footprint_radius_, 0.85))
        {
            RCLCPP_ERROR(node_->get_logger(), "SnapInCollisionGoals Failed to get local footprint, using default value of 0.85m");
        }
        max_snap_radius_ += footprint_radius_;
        goals_offset_ += footprint_radius_;

        RCLCPP_INFO(node_->get_logger(), "SnapInCollisionGoals successfully initialized!");
        
        initialized_ = true;
    }

    void SnapInCollisionGoalsAction::setup()
    {
        if (!initialized_)
        {
            initialize();
        }

        getInput("input_goals", input_goals_);

        if (!local_occu_grid_ || !global_occu_grid_)
        {
            wait_for_occu_grids();
        }

        RCLCPP_INFO(node_->get_logger(), "SnapInCollisionGoals successfully set up!");
        
        set_up_ = true;
    }
    
    void SnapInCollisionGoalsAction::halt()
    {
        toward_points_.clear();
        set_up_ = false;
    }

    inline BT::NodeStatus SnapInCollisionGoalsAction::tick()
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

        update_toward_points();
        // this is necessary to receive updates on the occupancy grids
        rclcpp::spin_some(node_);

        // snap!
        if (snap_goals())
        {
            return BT::NodeStatus::SUCCESS;
        }

        return BT::NodeStatus::FAILURE;
    }

    void SnapInCollisionGoalsAction::wait_for_occu_grids()
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
            node_->get_logger(), "SnapInCollisionGoals waited %.2fms for occupancy grids",
            std::chrono::duration<double, std::milli>(end - start).count()
        );
    }

    /**
     * @brief Remove toward points for removed goals and insert toward points for new goals.
     */
    void SnapInCollisionGoalsAction::update_toward_points()
    {
        // remove toward points for removed goals
        while (!toward_points_.empty() && !utils::nav2::areGoalsEqual(toward_points_[0].goal, input_goals_[0]))
        {
            RCLCPP_INFO(
                node_->get_logger(), "Removing toward point (%.2f, %.2f, %.2f) for goal (%.2f, %.2f, %.2f)",
                toward_points_[0].point.x, toward_points_[0].point.y, toward_points_[0].point.z,
                toward_points_[0].goal.pose.position.x, toward_points_[0].goal.pose.position.y, toward_points_[0].goal.pose.position.z
            );
            toward_points_.erase(toward_points_.begin());
        }
        // insert toward points for new goals
        for (size_t i = toward_points_.size(); i < input_goals_.size(); ++i)
        {
            TowardPoint toward_point{utils::nav2::offsetPose(input_goals_[i].pose, goals_offset_).position, input_goals_[i]};
            toward_points_.push_back(toward_point);
            RCLCPP_INFO(
                node_->get_logger(), "Adding toward point (%.2f, %.2f, %.2f) for goal (%.2f, %.2f, %.2f)",
                toward_point.point.x, toward_point.point.y, toward_point.point.z,
                input_goals_[i].pose.position.x, input_goals_[i].pose.position.y, input_goals_[i].pose.position.z
            );
        }
    }

    /**
     * @brief Core method of this node. Snaps goals that are in collision to the closest valid position.
     */
    bool SnapInCollisionGoalsAction::snap_goals()
    {
        unsigned int failed_snaps = 0;
        Goals output_goals_;

        for (size_t i = 0; i < input_goals_.size(); ++i)
        {
            Goal goal = input_goals_[i];
            SearchResult result = find_nearest_free_cell(goal.pose.position);

            if (!result.found)
            {
                failed_snaps += 1;
                RCLCPP_WARN(
                    node_->get_logger(), "Failed to snap goal (%.2f, %.2f, %.2f) to a free cell",
                    goal.pose.position.x, goal.pose.position.y, goal.pose.position.z
                );
                continue;
            }
            
            if (result.search_radius > 0)
            {
                Point original_pos = goal.pose.position;
                goal.pose.position = grid_cell_to_world(result.cell, global_occu_grid_);
                // reorient to corresponding toward point
                utils::nav2::orientTowards(goal.pose, toward_points_[i].point);
    
                RCLCPP_INFO(
                    node_->get_logger(), "Snapped goal (%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f)",
                    original_pos.x, original_pos.y, original_pos.z,
                    goal.pose.position.x, goal.pose.position.y, goal.pose.position.z
                );
                RCLCPP_INFO(
                    node_->get_logger(), "Original orientation: %d° Snapped orientation: %d°",
                    static_cast<int>(std::round(utils::nav2::degrees(tf2::getYaw(input_goals_[i].pose.orientation)))),
                    static_cast<int>(std::round(utils::nav2::degrees(tf2::getYaw(goal.pose.orientation))))
                );
            }
            
            output_goals_.push_back(goal);
            toward_points_[i].goal = goal;
        }

        setOutput("output_goals", output_goals_);

        if (failed_snaps > 0)
        {
            RCLCPP_WARN(node_->get_logger(), "Failed to snap %u goals", failed_snaps);
        }
        
        return failed_snaps == 0;
    }

    /**
     * @brief Find the nearest free cell using a simple spiral search. For every loop, the search
     * starts from the bottom left corner and goes clockwise.
     * 
     * @param origin The origin point to search around
     */
    SearchResult SnapInCollisionGoalsAction::find_nearest_free_cell(const Point &origin)
    {
        GridCell global_cell = world_to_grid_cell(origin, global_occu_grid_);

        // search for the nearest free cell in a spiral pattern
        std::array<int, 2> directions[4] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
        int max_radius = std::ceil(max_snap_radius_ / (*global_occu_grid_).info.resolution);
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
    bool SnapInCollisionGoalsAction::is_area_free(const GridCell &center)
    {
        // avoid extra computation if center cell is not free
        if (!is_cell_free(center))
        {
            return false;
        }

        int radius = std::ceil(footprint_radius_ / (*global_occu_grid_).info.resolution);
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

    /**
     * @brief Check if a cell is free in both the local and global occupancy grids
     * 
     * @param global_cell A cell with reference to the global occupancy grid
     */
    bool SnapInCollisionGoalsAction::is_cell_free(const GridCell &global_cell)
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
    bool SnapInCollisionGoalsAction::is_cell_free(const GridCell &cell, const OccupancyGrid::SharedPtr &grid)
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
    GridCell SnapInCollisionGoalsAction::world_to_grid_cell(const Point &point, const OccupancyGrid::SharedPtr &grid)
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
    Point SnapInCollisionGoalsAction::grid_cell_to_world(const GridCell &cell, const OccupancyGrid::SharedPtr &grid)
    {
        Point point;
        point.x = (*grid).info.origin.position.x + (cell.x * (*grid).info.resolution);
        point.y = (*grid).info.origin.position.y + (cell.y * (*grid).info.resolution);
        return point;
    }

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::SnapInCollisionGoalsAction>("SnapInCollisionGoals");
}