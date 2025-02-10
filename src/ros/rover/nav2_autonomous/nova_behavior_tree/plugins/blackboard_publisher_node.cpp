#include "nova_behavior_tree/blackboard_publisher_node.hpp"
#include "behaviortree_cpp/bt_factory.h"
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav_msgs/msg/path.hpp>


namespace nova_behavior_tree
{

BlackboardPublisherNode::BlackboardPublisherNode(
  const std::string & name,
  const BT::NodeConfiguration & config)
: BT::ActionNodeBase(name, config), topic_name_("blackboard_data")
{
  // Constructor: store defaults here. 
}

void BlackboardPublisherNode::initialize()
{
  // Grab the ROS node from the blackboard
  node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");

  // If "topic_name" was passed via an input port, override the default
  getInput("topic_name", topic_name_);

  // Create the publisher
  publisher_ = node_->create_publisher<std_msgs::msg::String>(
    topic_name_,
    rclcpp::QoS(10)
  );

  // Blackboard pointer
  bb = config().blackboard;

  // We assume your version of BehaviorTree.CPP provides getKeys() 
  // to list all the keys. If not, you'll have to store them manually.
  keys = bb->getKeys();
}

BT::NodeStatus BlackboardPublisherNode::tick()
{
  // Initialize once per activation
  if (!BT::isStatusActive(status())) {
    initialize();
  }

  // We'll build a string that lists each key and its value line-by-line
  std::string output;

  for (const auto & key : keys)
  {
    std::string value_str;
    bool found = false;

  // Hard Coded "goals" key
    if (!found && key == "goals") {
      try {
          auto goals = bb->get<std::vector<geometry_msgs::msg::PoseStamped>>(std::string(key));
          std::ostringstream ss;

          // Simplified formatting for testing
          ss << "Goals:";
          int idx = 1;
          for (const auto& goal_pose : goals) {
              ss << " [" << idx << "] ("
                << goal_pose.pose.position.x << ", "
                << goal_pose.pose.position.y << ")";
              idx++;
          }

          value_str = ss.str();  // Assign simplified string
          found = true;
      } catch(const std::exception& e) {
          value_str = std::string("Failed to retrieve goals: ") + e.what();
      } catch(...) {
          value_str = "Unknown error while retrieving goals.";
      }
    }

  // Hard Coded "path" key
    if (!found && key == "path") {
      try {
          auto path = bb->get<nav_msgs::msg::Path>(std::string(key));
          std::ostringstream ss;

          // Simplified formatting for testing
          ss << "Path:";
          int idx = 1;
          for (const auto& pose_stamped : path.poses) {
              ss << " [" << idx << "] ("
                << pose_stamped.pose.position.x << ", "
                << pose_stamped.pose.position.y << ")";
              idx++;
          }

          value_str = ss.str();  // Assign simplified string
          found = true;
      } catch(const std::exception& e) {
          value_str = std::string("Failed to retrieve path: ") + e.what();
      } catch(...) {
          value_str = "Unknown error while retrieving path.";
      }
    }




    // Try int
    try {
      int val = bb->get<int>(std::string(key));
      value_str = std::to_string(val);
      found = true;
    } catch(...) {}

    // Try double
    if (!found) {
      try {
        double val = bb->get<double>(std::string(key));
        value_str = std::to_string(val);
        found = true;
      } catch(...) {}
    }

    // Try bool
    if (!found) {
      try {
        bool val = bb->get<bool>(std::string(key));
        value_str = val ? "true" : "false";
        found = true;
      } catch(...) {}
    }

    // Try std::string
    if (!found) {
      try {
        std::string val = bb->get<std::string>(std::string(key));
        value_str = val;
        found = true;
      } catch(...) {}
    }
    //try node name
    if (!found) {
      try {
        auto node_ptr = bb->get<rclcpp::Node::SharedPtr>(std::string(key));
        value_str = std::string("Node Name: ") + node_ptr->get_name();
        found = true;
      } catch(...) {}
    }


    //Hard Coded "goal" key
    if (!found && key == "goal") {
      try {
        auto goal_pose = bb->get<geometry_msgs::msg::PoseStamped>("goal");
        std::ostringstream ss;
        ss << "Goal: ("
          << goal_pose.pose.position.x << ", "
          << goal_pose.pose.position.y << ", "
          << goal_pose.pose.position.z << ") "
          << "Orientation: ("
          << goal_pose.pose.orientation.x << ", "
          << goal_pose.pose.orientation.y << ", "
          << goal_pose.pose.orientation.z << ", "
          << goal_pose.pose.orientation.w << ")";
        value_str = ss.str();
        found = true;
      } catch(const std::exception& e) {
        value_str = std::string("Failed to retrieve goal: ") + e.what();
      } catch(...) {
        value_str = "Unknown error while retrieving goal.";
      }
    }

  





    // If we still didn't find a matching type, it's unsupported
    if (!found) {
      value_str = "UNSUPPORTED_TYPE";
    }

    // Append "key: value\n" to output
    output += std::string(key) + ": " + value_str + "\n";
  }

  // Publish our line-by-line string
  std_msgs::msg::String msg;
  msg.data = output.empty() ? "[No data in blackboard]" : output;
  publisher_->publish(msg);

  // Return SUCCESS every time for this demo
  return BT::NodeStatus::SUCCESS;
}

}  // namespace nova_behavior_tree

// Register to the factory so <BlackboardPublisher/> is recognized in XML
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::BlackboardPublisherNode>("BlackboardPublisher");
}
