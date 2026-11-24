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
// EXERCISE 1: Declare the node's ports.
// ---------------------------------------------------------------------------
BT::PortsList GoalDistanceFilter::providedPorts()
{
  // TODO: return a list of the node's three ports:
  //   - an input  "goal"         of type geometry_msgs::msg::PoseStamped
  //   - an input  "max_distance" of type double, with a default of 10.0
  //   - an output "distance"     of type double
  // Each port can also take a short description string as its last argument.

  // Hint:BT::InputPort<*Type*>(name, default_value (optional), description)
  //       BT::OutputPort<*Type*>(name, description)
  return {
    // --- WRITE YOUR SOLUTION HERE ---
    BT::___<___>(___,___),
    ___,
    ___
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
  auto goal = getInput<*TYPE*>(*NAME*);
  if (!goal) {
    RCLCPP_ERROR(node_->get_logger(), "[GoalDistanceFilter] could not read 'goal' input");
    // What status should we return if we didn't manage to get an input goal, SUCCESS or FAILURE?
    return BT::NodeStatus::___; 
  }

  // Now do the same for max_distance.
  // You will need to check the type you declared it as in Exercise 1, as well as its port name.
  auto ___ = ___;
  if (___) {
    RCLCPP_ERROR(node_->get_logger(), "___");
    return ___;
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
  const *TYPE* distance = ___;

  // Hint: What is the name of the output port, and what value should we write to it?
  setOutput("___", ___);


  // ---------------------------------------------------------------------------
  // EXERCISE 4: Decide if the goal is valid or not
  // ---------------------------------------------------------------------------

  // TODO:
  //   - If distance is greater than max_distance, the goal is too far:
  //     call the provided removeGoal() helper, then return FAILURE.
  //   - Otherwise do nothing: the goal is allowed, and the SUCCESS below
  //     is returned
  
  // --- WRITE YOUR SOLUTION HERE ---

  if (___ > ___.value()) {
    RCLCPP_ERROR(node_->get_logger(), "[GoalDistanceFilter] Goal too far");
    removeGoal();
    return BT::NodeStatus::___;
  }
  return BT::NodeStatus::___;

}

// ---------------------------------------------------------------------------
// EXERCISE 5: Implement a basic helper function to compute the distance between two Points
// ---------------------------------------------------------------------------

double GoalDistanceFilter::computeDistance(
  const geometry_msgs::msg::Point & a,
  const geometry_msgs::msg::Point & b)
{
  // Initialise the distance variable
  double ___ = ___;

  // Set distance to the 2D (x, y) distance between a and b.
  // Hint: use std::hypot(*distance between x coords*, *distance between y coords*)
  // https://manuals.quinngrier.com/cppreference/20140208/reference/en/cpp/numeric/math/hypot.html

  ___ = std::hypot(___, ___);
  return ___;
}

// ---------------------------------------------------------------------------
// END OF EXERCISES
// ---------------------------------------------------------------------------




// ---------------------------------------------------------------------------
// TF lookup
// ---------------------------------------------------------------------------
bool GoalDistanceFilter::getRobotPosition(
  const std::string & frame,
  geometry_msgs::msg::Point & position)
{
  // Get the TF buffer from the blackboard .
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
// Goal removal
// ---------------------------------------------------------------------------
void GoalDistanceFilter::removeGoal()
{
  // The port value looks like "{goal} (a dict)". Strip the braces to get the key.
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
