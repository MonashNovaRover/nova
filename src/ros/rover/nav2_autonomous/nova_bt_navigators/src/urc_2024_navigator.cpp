// Copyright (c) 2021 Samsung Research
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

#include <vector>
#include <string>
#include <set>
#include <memory>
#include <limits>
#include "nova_bt_navigators/urc_2024_navigator.hpp"

namespace nova_bt_navigators
{

bool
URC2024Navigator::configure(
  rclcpp_lifecycle::LifecycleNode::WeakPtr parent_node,
  std::shared_ptr<nav2_util::OdomSmoother> odom_smoother)
{
  start_time_ = rclcpp::Time(0);
  std::shared_ptr<rclcpp_lifecycle::LifecycleNode> node = parent_node.lock();


  if (!node->has_parameter("goals_blackboard_id")) {
    node->declare_parameter("goals_blackboard_id", std::string("goals"));
  }

  goals_blackboard_id_ = node->get_parameter("goals_blackboard_id").as_string();

  if (!node->has_parameter("path_blackboard_id")) {
    node->declare_parameter("path_blackboard_id", std::string("path"));
  }

  path_blackboard_id_ = node->get_parameter("path_blackboard_id").as_string();

  if (!node->has_parameter("global_frame_id")) {
      node->declare_parameter("global_frame_id", std::string("map"));
  }

  global_frame_id_ = node->get_parameter("global_frame_id").as_string();

  from_ll_to_map_client_ = std::make_unique<
    nav2_util::ServiceClient<robot_localization::srv::FromLL,
    std::shared_ptr<rclcpp_lifecycle::LifecycleNode>>>(
    "/fromLL",
    node);

  // Odometry smoother object for getting current speed
  odom_smoother_ = odom_smoother;

  return true;
}

bool
URC2024Navigator::cleanup()
{
  from_ll_to_map_client_.reset();
  return true;
}

std::string
URC2024Navigator::getDefaultBTFilepath(
  rclcpp_lifecycle::LifecycleNode::WeakPtr parent_node)
{
  std::string default_bt_xml_filename;
  auto node = parent_node.lock();

  if (!node->has_parameter("default_urc_2024_navigator_bt_xml")) {
    std::string pkg_share_dir =
      ament_index_cpp::get_package_share_directory("nova_behavior_tree");
    node->declare_parameter<std::string>(
      "default_urc_2024_navigator_bt_xml",
      pkg_share_dir +
      "/behavior_tree/urc/urc_through_poses_search.xml");
  }

  node->get_parameter("default_urc_2024_navigator_bt_xml", default_bt_xml_filename);

  return default_bt_xml_filename;
}

bool
URC2024Navigator::goalReceived(ActionT::Goal::ConstSharedPtr goal)
{
  auto bt_xml_filename = goal->behavior_tree;

  if (!bt_action_server_->loadBehaviorTree(bt_xml_filename)) {
    RCLCPP_ERROR(
      logger_, "Error loading XML file: %s. Navigation canceled.",
      bt_xml_filename.c_str());
    return false;
  }

  // #TODO: move initializeGoalPose logic into here as we don't support pre-emption.

  auto map_poses = convertGPSPosesToMapPoses(goal->gps_poses);

  if (!map_poses.has_value()){
    RCLCPP_ERROR(
      logger_, "Conversion of gps to map goals was unsuscessful. Navigation canceled."
    );
    return false;
  }

  //#TODO Check that distance between goals is less than size of global costmap / 2

  initializeGoalPose(goal, map_poses.value());

  return true;
}

void
URC2024Navigator::goalCompleted(
  typename ActionT::Result::SharedPtr /*result*/,
  const nav2_behavior_tree::BtStatus /*final_bt_status*/)
{
}

void
URC2024Navigator::onLoop()
{
  using namespace nav2_util::geometry_utils;  // NOLINT

  // action server feedback (pose, duration of task,
  // number of recoveries, and distance remaining to goal, etc)
  auto feedback_msg = std::make_shared<ActionT::Feedback>();

  auto blackboard = bt_action_server_->getBlackboard();

  Goals goal_poses;
  blackboard->get<Goals>(goals_blackboard_id_, goal_poses);

  feedback_msg->waypoints_remaining = goal_poses.size();

  // SearchGoals search_goals;
  // if (blackboard->get<SearchGoals>(search_goals_id_, search_goals)) {
  //   feedback_msg->phase = feedback_msg->SEARCH;
  // } else {
  //   feedback_msg->phase = feedback_msg->NAVIGATION;
  // }

  int recovery_count = 0;
  blackboard->get<int>("number_recoveries", recovery_count);
  feedback_msg->number_of_recoveries = recovery_count;
  feedback_msg->navigation_time = clock_->now() - start_time_;

  bt_action_server_->publishFeedback(feedback_msg);
}

void
URC2024Navigator::onPreempt(ActionT::Goal::ConstSharedPtr goal)
{
    // #TODO: look into preemption to see if we can/should support it.
    RCLCPP_WARN(
      logger_,
      "Preemption is unsupported. Cancel goal and send a new one");
    bt_action_server_->terminatePendingGoal();
}

void
URC2024Navigator::initializeGoalPose(ActionT::Goal::ConstSharedPtr goal,
                                 const std::vector<geometry_msgs::msg::PoseStamped> & map_poses)
{
  if (goal->gps_poses.size() > 0) {
    RCLCPP_INFO(
      logger_, "Begin navigating from current location through %zu poses to (latitude: %.2f, longitude %.2f)",
      goal->gps_poses.size(), goal->gps_poses.back().position.latitude, goal->gps_poses.back().position.longitude);
  }

  // Reset state for new action feedback
  start_time_ = clock_->now();
  auto blackboard = bt_action_server_->getBlackboard();
  blackboard->set<int>("number_recoveries", 0);  // NOLINT

  // Update the goal pose on the blackboard
  blackboard->set<Goals>(goals_blackboard_id_, map_poses);

  //update detection type and id
  blackboard->set<std::string>("detection_type", goal->detection_type);
  blackboard->set<std::string>("detection_id", goal->detection_id);
}

std::optional<std::vector<geometry_msgs::msg::PoseStamped>>
URC2024Navigator::convertGPSPosesToMapPoses(
  const std::vector<geographic_msgs::msg::GeoPose> & gps_poses)
{
  RCLCPP_INFO(
    logger_, "Converting GPS goals to %s Frame..",
    global_frame_id_.c_str());

  std::vector<geometry_msgs::msg::PoseStamped> poses_in_map_frame_vector;
  int goal_index = 0;
  for (auto && curr_geopose : gps_poses) {
    auto request = std::make_shared<robot_localization::srv::FromLL::Request>();
    auto response = std::make_shared<robot_localization::srv::FromLL::Response>();
    request->ll_point.latitude = curr_geopose.position.latitude;
    request->ll_point.longitude = curr_geopose.position.longitude;
    request->ll_point.altitude = curr_geopose.position.altitude;

    from_ll_to_map_client_->wait_for_service((std::chrono::seconds(1)));
    if (!from_ll_to_map_client_->invoke(request, response)) {
        RCLCPP_ERROR(
          logger_,
          "Conversion of %i th GPS goal to"
          "%s frame failed",
          goal_index, global_frame_id_.c_str());
        return std::nullopt;
    } else {
        RCLCPP_INFO_STREAM(
                logger_,
                "GPS Goal" << goal_index <<
                "with latitude: " << curr_geopose.position.latitude <<
                ", longitude: " << curr_geopose.position.longitude <<
                "was converted to map coordinates of x: " << response->map_point.x <<
                ", y: " << response->map_point.y;
        );
        geometry_msgs::msg::PoseStamped curr_pose_map_frame;
        curr_pose_map_frame.header.frame_id = global_frame_id_;
        curr_pose_map_frame.header.stamp = clock_->now();
        curr_pose_map_frame.pose.position = response->map_point;
        curr_pose_map_frame.pose.orientation = curr_geopose.orientation;
        poses_in_map_frame_vector.push_back(curr_pose_map_frame);
    }
    goal_index++;
  }
  RCLCPP_INFO(
    logger_,
    "Converted all %i GPS goals to %s frame",
    static_cast<int>(poses_in_map_frame_vector.size()), global_frame_id_.c_str());
  return poses_in_map_frame_vector;
}

}  // namespace nova_bt_navigators

#include "pluginlib/class_list_macros.hpp"
PLUGINLIB_EXPORT_CLASS(
  nova_bt_navigators::URC2024Navigator,
  nav2_core::NavigatorBase)