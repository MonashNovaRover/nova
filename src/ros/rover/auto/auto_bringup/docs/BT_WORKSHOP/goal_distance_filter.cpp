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
#include <iostream>
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
// PROVIDED: constructor. Nothing to implement here.
// ---------------------------------------------------------------------------
GoalDistanceFilter::GoalDistanceFilter(
  const std::string & name,
  const BT::NodeConfiguration & config)
: BT::SyncActionNode(name, config)
{
}

// ---------------------------------------------------------------------------
// EXERCISE 1: declare the node's ports.
// ---------------------------------------------------------------------------
BT::PortsList GoalDistanceFilter::providedPorts()
{
  // TODO(student): return a list of the node's three ports:
  //   - an input  "goal"         of type geometry_msgs::msg::PoseStamped
  //   - an input  "max_distance" of type double, with a default of 100.0
  //   - an output "distance"     of type double
  // Each port can also take a short description string as its last argument.
  // Hint: BT::InputPort<T>(name, default_value, description),
  //       BT::OutputPort<T>(name, description)
  return {
    // --- BEGIN SOLUTION ---
    BT::InputPort<geometry_msgs::msg::PoseStamped>(
      "goal", "Goal pose to check (removed from the blackboard if too far)"),
    BT::InputPort<double>(
      "max_distance", 10.0, "Furthest allowed robot-to-goal distance (m)"),
    BT::OutputPort<double>(
      "distance", "Measured robot-to-goal distance (m)")
    // --- END SOLUTION ---
  };
}

// ---------------------------------------------------------------------------
// EXERCISES 2-4: the tick.
// ---------------------------------------------------------------------------
BT::NodeStatus GoalDistanceFilter::tick()
{
  // TODO(student) Exercise 2: read the inputs.
  //   - Read "goal" and "max_distance" with getInput<T>().
  //   - getInput returns BT::Expected<T>. Check it before using it:
  //     if (!result) { ... } then use result.value() to get the real value.
  //   - If an input can't be read, print result.error() and return FAILURE.
  //     Why FAILURE, and not SUCCESS?
  // --- BEGIN SOLUTION ---
  auto goal = getInput<geometry_msgs::msg::PoseStamped>("goal");
  if (!goal) {
    std::cerr << "[GoalDistanceFilter] '" << name() << "' could not read 'goal': " <<
      goal.error() << std::endl;
    return BT::NodeStatus::FAILURE;
  }

  auto max_distance = getInput<double>("max_distance");
  if (!max_distance) {
    std::cerr << "[GoalDistanceFilter] '" << name() << "' could not read 'max_distance': " <<
      max_distance.error() << std::endl;
    return BT::NodeStatus::FAILURE;
  }
  // --- END SOLUTION ---

  // TODO(student) Exercise 3: measure the distance and write the output.
  //   - Call the provided getRobotPosition(frame, position) helper. The frame
  //     to use is the goal's frame: goal.value().header.frame_id.
  //     If it returns false, return FAILURE (we can't tell how far away the
  //     goal is, so we shouldn't remove it).
  //   - Call computeDistance() with the robot's position and the goal's
  //     position (goal.value().pose.position).
  //   - Write the result to the "distance" output port with setOutput().
  // --- BEGIN SOLUTION ---
  geometry_msgs::msg::Point robot_position;
  if (!getRobotPosition(goal.value().header.frame_id, robot_position)) {
    return BT::NodeStatus::FAILURE;
  }

  const double distance = computeDistance(robot_position, goal.value().pose.position);
  setOutput("distance", distance);
  // --- END SOLUTION ---

  // TODO(student) Exercise 4: decide the status.
  //   - If distance is greater than max_distance, the goal is too far:
  //     call the provided removeGoal() helper, then return FAILURE.
  //   - Otherwise do nothing: the goal is allowed, and the SUCCESS below
  //     is returned.
  // Discussion: what happens higher up the tree when this returns FAILURE?
  // --- BEGIN SOLUTION ---
  if (distance > max_distance.value()) {
    std::cout << "[GoalDistanceFilter] '" << name() << "' goal too far: " << distance <<
      " m > " << max_distance.value() << " m, removing goal" << std::endl;
    removeGoal();
    return BT::NodeStatus::FAILURE;
  }
  // --- END SOLUTION ---

  return BT::NodeStatus::SUCCESS;
}

// ---------------------------------------------------------------------------
// EXERCISE 5: a plain C++ helper (no BT knowledge needed).
// ---------------------------------------------------------------------------
double GoalDistanceFilter::computeDistance(
  const geometry_msgs::msg::Point & a,
  const geometry_msgs::msg::Point & b)
{
  double distance = 0.0;
  // TODO(student): set distance to the 2D (x, y) distance between a and b.
  // Hint: std::hypot(dx, dy) from <cmath>.
  // --- BEGIN SOLUTION ---
  distance = std::hypot(b.x - a.x, b.y - a.y);
  // --- END SOLUTION ---
  return distance;
}

// ---------------------------------------------------------------------------
// PROVIDED: TF lookup. Not part of the exercise.
// ---------------------------------------------------------------------------
bool GoalDistanceFilter::getRobotPosition(
  const std::string & frame,
  geometry_msgs::msg::Point & position)
{
  // Get the TF buffer from the blackboard (looked up once, then cached).
  if (!tf_buffer_) {
    if (!config().blackboard->get("tf_buffer", tf_buffer_) || !tf_buffer_) {
      std::cerr << "[GoalDistanceFilter] '" << name() <<
        "' could not find 'tf_buffer' on the blackboard" << std::endl;
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
    std::cerr << "[GoalDistanceFilter] '" << name() << "' TF lookup failed: " <<
      ex.what() << std::endl;
    return false;
  }

  return true;
}

// ---------------------------------------------------------------------------
// PROVIDED: goal removal. Not part of the exercise.
// ---------------------------------------------------------------------------
void GoalDistanceFilter::removeGoal()
{
  // The port value looks like "{goal}". Strip the braces to get the key.
  const auto it = config().input_ports.find("goal");
  BT::StringView key;
  if (it == config().input_ports.end() ||
    !BT::TreeNode::isBlackboardPointer(it->second, &key))
  {
    std::cerr << "[GoalDistanceFilter] '" << name() <<
      "' could not remove goal: 'goal' port is not mapped to a blackboard entry" <<
      std::endl;
    return;
  }

  config().blackboard->unset(std::string(key));
}

}  // namespace nova_behavior_tree

// ---------------------------------------------------------------------------
// PROVIDED: registration. Not part of the exercise.
// ---------------------------------------------------------------------------
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::GoalDistanceFilter>("GoalDistanceFilter");
}
