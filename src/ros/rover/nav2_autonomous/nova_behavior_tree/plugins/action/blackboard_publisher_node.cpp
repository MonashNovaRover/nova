#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#include "behaviortree_cpp/bt_factory.h"
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav_msgs/msg/path.hpp>

#include "nova_behavior_tree/action/blackboard_publisher_node.hpp"

namespace nova_behavior_tree
{

BlackboardPublisherNode::BlackboardPublisherNode(
  const std::string & name,
  const BT::NodeConfiguration & config)
: BT::ActionNodeBase(name, config), topic_name_("blackboard")
{
  // Constructor: store defaults here. 
}

void BlackboardPublisherNode::initialize()
{
  // Grab the ROS node from the blackboard
  node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");

  // If "topic_name" was passed via an input port, override the default
  getInput("topic_name", topic_name_);
  
  double publish_frequency;
  getInput("publish_frequency", publish_frequency);
  // publish_delay_ is in ms
  publish_delay_ = 1000.0 / publish_frequency;

  getInput("keys", keys_string_);
  split_key_string();

  // Create the publisher
  publisher_ = node_->create_publisher<std_msgs::msg::String>(
    topic_name_,
    rclcpp::QoS(10)
  );

  // Blackboard pointer
  bb_ = config().blackboard;

  last_publish_ = std::chrono::steady_clock::now();

  initialized_ = true;
}

BT::NodeStatus BlackboardPublisherNode::tick()
{
  // Initialize once per activation
  if (!initialized_) {
    initialize();
  }

  auto now = std::chrono::steady_clock::now();
  auto time_since_last_publish = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_publish_).count();
  if (time_since_last_publish > publish_delay_)
  {
    last_publish_ = now;
    publish_blackboard();
    RCLCPP_DEBUG(node_->get_logger(), "Blackboard published!");
  }

  // Return SUCCESS every time for this demo
  return BT::NodeStatus::SUCCESS;
}

void BlackboardPublisherNode::split_key_string()
{
  std::stringstream ss(keys_string_);
  std::string key;

  while (std::getline(ss, key, ','))
  {
    keys_.push_back(key);
  }
}

void BlackboardPublisherNode::publish_blackboard()
{
  // We'll build a string that lists each key and its value line-by-line
  std::string output;
  for (const auto &key : keys_)
  {
    std::string value_str;
    bool found = false;

    // current_pose publisher
    if (!found && key == "current_pose") {
      try {
        auto p = bb_->get<geometry_msgs::msg::PoseStamped>(key);
        std::ostringstream ss;
        ss << "("
          << p.pose.position.x << ","
          << p.pose.position.y << ","
          << p.pose.position.z << ")";
        value_str = ss.str();
        found = true;
      } catch(...) {
        value_str = "Failed to retrieve PoseStamped";
      }
    }

    // Hard Coded "goals" key
    if (!found && key == "goals") {
      try {
          auto goals = bb_->get<std::vector<geometry_msgs::msg::PoseStamped>>(key);
          std::ostringstream ss;

          // Simplified formatting for testing
          for (const auto& goal_pose : goals) {
              ss << "("
                << goal_pose.pose.position.x << ", "
                << goal_pose.pose.position.y << ", "
                << goal_pose.pose.position.z << ", "
                << goal_pose.pose.orientation.x << ", "
                << goal_pose.pose.orientation.y << ", "
                << goal_pose.pose.orientation.z << ", "
                << goal_pose.pose.orientation.w << ")";
          }

          value_str = ss.str();  // Assign simplified string
          found = true;
      } catch(const std::exception& e) {
          value_str = std::string("Failed to retrieve goals: ") + e.what();
      } catch(...) {
          value_str = "Unknown error while retrieving goals.";
      }
    }

    // Try int
    try {
      int val = bb_->get<int>(key);
      value_str = std::to_string(val);
      found = true;
    } catch(...) {}

    // Try double
    if (!found) {
      try {
        double val = bb_->get<double>(key);
        value_str = std::to_string(val);
        found = true;
      } catch(...) {}
    }

    // Try bool
    if (!found) {
      try {
        bool val = bb_->get<bool>(key);
        value_str = val ? "true" : "false";
        found = true;
      } catch(...) {}
    }

    // Try std::string
    if (!found) {
      try {
        std::string val = bb_->get<std::string>(key);
        value_str = val;
        found = true;
      } catch(...) {}
    }
    //try node name
    if (!found) {
      try {
        auto node_ptr = bb_->get<rclcpp::Node::SharedPtr>(key);
        value_str = std::string("Node Name: ") + node_ptr->get_name();
        found = true;
      } catch(...) {}
    }

    // If we still didn't find a matching type, it's unsupported
    if (!found) {
      value_str = "UNSUPPORTED_TYPE";
    }

    // Append "key: value\n" to output
    output += key + ": " + value_str + "\n";
  }

  // Publish our line-by-line string
  std_msgs::msg::String msg;
  msg.data = output.empty() ? "[No data in blackboard]" : output;
  publisher_->publish(msg);
}

}  // namespace nova_behavior_tree

// Register to the factory so <BlackboardPublisher/> is recognized in XML
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::BlackboardPublisherNode>("BlackboardPublisher");
}
