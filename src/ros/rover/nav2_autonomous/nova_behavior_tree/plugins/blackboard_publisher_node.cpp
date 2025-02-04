#include "nova_behavior_tree/blackboard_publisher_node.hpp"
#include "behaviortree_cpp/bt_factory.h"

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
