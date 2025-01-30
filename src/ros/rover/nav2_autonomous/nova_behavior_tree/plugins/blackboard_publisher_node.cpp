#include <string>
#include <memory>
#include "nova_behavior_tree/blackboard_publisher_node.hpp"
#include "behaviortree_cpp/bt_factory.h"

namespace nova_behavior_tree
{

BlackboardPublisherNode::BlackboardPublisherNode(
  const std::string & xml_tag_name,
  const BT::NodeConfiguration & conf)
: BT::ActionNodeBase(xml_tag_name, conf), topic_name_("blackboard_data")
{
  // Initialize parameters from XML input ports (if any)
}

void BlackboardPublisherNode::initialize()
{
  // Get ROS node handle from the blackboard
  node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
  
  // Get topic name from input port (or use default)
  getInput("topic_name", topic_name_);
  
  // Initialize publisher
  publisher_ = node_->create_publisher<std_msgs::msg::String>(
    topic_name_, rclcpp::SystemDefaultsQoS());
}

BT::NodeStatus BlackboardPublisherNode::tick()
{
  if (!BT::isStatusActive(status())) {
    initialize();
  }

  // Get all entries from the Blackboard
  const auto& blackboard = config().blackboard;
  auto entries = blackboard->entries();

  // Convert to JSON-like string
  std::string json_str = "{";
  for (const auto& [key, value] : entries) {
    json_str += "\"" + key + "\":\"" + value.value().toString() + "\",";
  }
  if (!entries.empty()) json_str.pop_back();  // Remove trailing comma
  json_str += "}";

  // Publish to ROS
  std_msgs::msg::String msg;
  msg.data = json_str;
  publisher_->publish(msg);

  return BT::NodeStatus::SUCCESS;
}

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::BlackboardPublisherNode>("BlackboardPublisher");
}