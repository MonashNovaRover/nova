#include "nova_behavior_tree/blackboard_publisher_node.hpp"
#include "behaviortree_cpp/bt_factory.h"

namespace nova_behavior_tree
{

// Hard-coded keys from your XML (and possibly Nav2 internals).
// Add or remove as needed if you discover new keys.
static const std::vector<std::string> KNOWN_KEYS = {
  "node",
  "selected_controller",
  "selected_planner",
  "seen_ids",
  "id",
  "goal",
  "goals",
  "path",
  "compute_path_error_code",
  "follow_path_error_code",
  "spin_error_code",
  "backup_code_id"
};

BlackboardPublisherNode::BlackboardPublisherNode(
  const std::string & name,
  const BT::NodeConfiguration & config)
: BT::ActionNodeBase(name, config), topic_name_("blackboard_data")
{
  // Store default topic name. The actual Node/Publisher init is in initialize().
}

void BlackboardPublisherNode::initialize()
{
  // Retrieve ROS node from the blackboard
  node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");

  // Override default topic name if provided in XML
  getInput("topic_name", topic_name_);

  // Create a simple std_msgs/String publisher
  publisher_ = node_->create_publisher<std_msgs::msg::String>(topic_name_, rclcpp::QoS(10));
}

BT::NodeStatus BlackboardPublisherNode::tick()
{
  // If transitioning from IDLE to RUNNING, initialize once
  if (!BT::isStatusActive(status())) {
    initialize();
  }

  auto bb = config().blackboard;
  std::string output;

  // Loop over our known keys
  for (const auto & key : KNOWN_KEYS)
  {
    // We'll attempt to read the key. 
    // If it doesn't exist or has a mismatching type, get<T>() throws an exception.

    bool found = false;
    std::string value_str;

    // Try int
    try {
      int val = bb->get<int>(key);
      value_str = std::to_string(val);
      found = true;
    } catch(...) {}

    // Try double
    if (!found) {
      try {
        double val = bb->get<double>(key);
        value_str = std::to_string(val);
        found = true;
      } catch(...) {}
    }

    // Try bool
    if (!found) {
      try {
        bool val = bb->get<bool>(key);
        value_str = val ? "true" : "false";
        found = true;
      } catch(...) {}
    }

    // Try std::string
    if (!found) {
      try {
        std::string val = bb->get<std::string>(key);
        value_str = val;
        found = true;
      } catch(...) {}
    }

    // If we never found a match, the key might not exist or is in an unsupported type
    if (!found) {
      value_str = "NOT_FOUND_OR_UNSUPPORTED_TYPE";
    }

    // Append "key: value" to the output
    output += key + ": " + value_str + "\n";
  }

  // If none of those keys existed or all were unsupported, output might be blank
  if (output.empty()) {
    output = "[No recognized keys found]";
  }

  // Publish the combined string
  std_msgs::msg::String msg;
  msg.data = output;
  publisher_->publish(msg);

  // Return SUCCESS each tick so we keep publishing
  return BT::NodeStatus::SUCCESS;
}

}  // namespace nova_behavior_tree

// Register your node so <BlackboardPublisher .../> is recognized in XML
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::BlackboardPublisherNode>("BlackboardPublisher");
}
