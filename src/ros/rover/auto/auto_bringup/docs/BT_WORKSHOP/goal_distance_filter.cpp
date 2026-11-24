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

#include "nova_behavior_tree/action/goal_distance_filter.hpp"

#include <cmath>
#include <memory>
#include <string>

#include "tf2/exceptions.h"
#include "tf2/time.h"

namespace nova_behavior_tree
{

namespace
{
// Frame used to find the robot's position.
constexpr char kRobotBaseFrame[] = "base_link";
}  // namespace

// ---------------------------------------------------------------------------
// Constructor + initializer
// ---------------------------------------------------------------------------
GoalDistanceFilter::GoalDistanceFilter(
  const std::string & name,
  const BT::NodeConfiguration & config)
: BT::SyncActionNode(name, config)
{
  initialize();
}

void GoalDistanceFilter::initialize()
{
  node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
  RCLCPP_INFO(node_->get_logger(), "GoalDistanceFilter successfully initialized!");
}

// ---------------------------------------------------------------------------
// EXERCISE 1: declare the node's ports.
// ---------------------------------------------------------------------------
BT::PortsList GoalDistanceFilter::providedPorts()
{
  // TODO: return a list of the node's three ports:
  //   - an input  "goal"         of type geometry_msgs::msg::PoseStamped
  //   - an input  "max_distance" of type double, with a default of 10.0
  //   - an output "distance"     of type double
  // Each port can also take a short description string as its last argument.
  // Hint: BT::InputPort<T>(name, default_value, description),
  //       BT::OutputPort<T>(name, description)
  return {
    // --- BEGIN SOLUTION ---
    BT::InputPort<geometry_msgs::msg::PoseStamped>("goal", "Goal pose to check"),
    BT::InputPort<double>("max_distance", 10.0, "Furthest allowed robot-to-goal distance (m)"),
    BT::OutputPort<double>("distance", "Measured robot-to-goal distance (m)")
  };
}

// ---------------------------------------------------------------------------
// EXERCISE 2: on Tick function
// ---------------------------------------------------------------------------
BT::NodeStatus GoalDistanceFilter::tick()
{
 // TODO: read and validate the inputs
  //   - Read "goal" and "max_distance" with getInput<Type>(....).
  //   - Check if (!result) { ... }
  //   - If an input can't be read, print an error message and return a status for the node.

  // --- WRITE YOUR SOLUTION HERE ---
  auto goal = getInput<geometry_msgs::msg::PoseStamped>("goal");
  if (!goal) {
    RCLCPP_ERROR(node_->get_logger(), "[GoalDistanceFilter] could not read 'goal' input");
    return BT::NodeStatus::FAILURE;
  }

  double max_distance = getInput<double>("max_distance");
  if (!max_distance) {
    RCLCPP_ERROR(node_->get_logger(), "[GoalDistanceFilter] could not read 'max_distance' input");
    return BT::NodeStatus::FAILURE;
  }

  // ---------------------------------------------------------------------------
  // EXERCISE 3: Measure the distance and write the output
  // ---------------------------------------------------------------------------
  
  // First we check to make sure we can actually get the position of the goal and the robot
  // Side Tangent (Ros2 Geometry Msgs)
  // https://docs.ros2.org/foxy/api/geometry_msgs/index-msg.html 

  geometry_msgs::msg::Point robot_position;
  if (!getRobotPosition(goal.value().header.frame_id, robot_position)) {
    return BT::NodeStatus::FAILURE;
  }

  // TODO:
  //   - Call computeDistance() with the robot's position and the goal's position (goal.value().pose.position).
  //   - Write the result to the "distance" output port with setOutput().
  
  // --- WRITE YOUR SOLUTION HERE ---
  // Hint: Check out the computeDistance function to see what it expects as an input, and the type it returns.
  const double distance = computeDistance(robot_position, goal.value().pose.position);
  setOutput("distance", distance);

  // ---------------------------------------------------------------------------
  // EXERCISE 4: Decide if the goal is valid or not
  // ---------------------------------------------------------------------------

  // TODO:
  //   - If distance is greater than max_distance, the goal is too far:
  //     call the provided removeGoal() helper, then return FAILURE.
  //   - Otherwise do nothing: the goal is allowed, and the SUCCESS below
  //     is returned
  
  // --- WRITE YOUR SOLUTION HERE ---
  if (distance > max_distance.value()) {
    RCLCPP_ERROR(node_->get_logger(), "[GoalDistanceFilter] Goal too far: %.2f m > %.2f m", distance, max_distance);
    removeGoal();
    return BT::NodeStatus::FAILURE;
  }
  return BT::NodeStatus::SUCCESS;
}

// ---------------------------------------------------------------------------
// EXERCISE 5: C++ helper (no BT knowledge needed).
// ---------------------------------------------------------------------------
double GoalDistanceFilter::computeDistance(
  const geometry_msgs::msg::Point & a,
  const geometry_msgs::msg::Point & b)
{
  double distance = 0.0;
  // TODO(student): set distance to the 2D (x, y) distance between a and b.
  // Hint: std::hypot(dx, dy) from <cmath>
  // https://manuals.quinngrier.com/cppreference/20140208/reference/en/cpp/numeric/math/hypot.html 

  distance = std::hypot(b.x - a.x, b.y - a.y);
  return distance;
}

// ---------------------------------------------------------------------------
// END OF EXERCISES
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
// get robot position
// ---------------------------------------------------------------------------

bool GoalDistanceFilter::getRobotPosition(
  const std::string & frame,
  geometry_msgs::msg::Point & position)
{
  // Get the TF buffer from the blackboard (looked up once, then cached).
  if (!tf_buffer_) {
    if (!config().blackboard->get("tf_buffer", tf_buffer_) || !tf_buffer_) {
      RCLCPP_ERROR(node_->get_logger(), "[GoalDistanceFilter] Could not find 'tf_buffer' on the blackboard");
      return false;
    }
  }

  // Find where the robot is, expressed in the given frame.
  try {
    const auto transform = tf_buffer_->lookupTransform(
      frame, kRobotBaseFrame, tf2::TimePointZero);
    position.x = transform.transform.translation.x;
    position.y = transform.transform.translation.y;
  } catch (const tf2::TransformException & ex) {
    RCLCPP_ERROR(node_->get_logger(), "[GoalDistanceFilter] TF lookup failed");
    return false;
  }

  return true;
}

// ---------------------------------------------------------------------------
// goal removal
// ---------------------------------------------------------------------------
void GoalDistanceFilter::removeGoal()
{
  // The port value looks like "{goal}". Strip the braces to get the key.
  const auto it = config().input_ports.find("goal");
  BT::StringView key;
  if (it == config().input_ports.end() ||
    !BT::TreeNode::isBlackboardPointer(it->second, &key))
  {
    RCLCPP_ERROR(node_->get_logger(), "[GoalDistanceFilter] could not remove goal: 'goal' port is not mapped to a blackboard entry");
    return;
  }

  config().blackboard->unset(std::string(key));
}

}  // namespace nova_behavior_tree

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::GoalDistanceFilter>("GoalDistanceFilter");
}
