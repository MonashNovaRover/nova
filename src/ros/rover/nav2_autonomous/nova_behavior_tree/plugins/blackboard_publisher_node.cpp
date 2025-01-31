#include "nova_behavior_tree/blackboard_publisher_node.hpp"
#include "behaviortree_cpp/bt_factory.h"

namespace nova_behavior_tree
{

BlackboardPublisherNode::BlackboardPublisherNode(
  const std::string & name,
  const BT::NodeConfiguration & config)
: BT::ActionNodeBase(name, config), topic_name_("blackboard_data")
{
  // The constructor simply stores defaults. 
  // We do actual ROS setup in initialize() to ensure we have the blackboard.
}

void BlackboardPublisherNode::initialize()
{
  // Retrieve the shared pointer to the ROS 2 node from the blackboard
  node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");

  // If "topic_name" was passed as an input port, override the default:
  getInput("topic_name", topic_name_);

  // Create a publisher for std_msgs/String messages
  publisher_ = node_->create_publisher<std_msgs::msg::String>(
    topic_name_,
    rclcpp::QoS(10)  // Or SystemDefaultsQoS(), etc.
  );
}

BT::NodeStatus BlackboardPublisherNode::tick()
{
  // If this node is not yet active, do the one-time setup
  if (!BT::isStatusActive(status())) {
    initialize();
  }

  // Here is where you can read from the blackboard if you wish.
  // For this minimal example, we simply publish a fixed string.
  std_msgs::msg::String msg;
  msg.data = "WE ARE READING THE BLACKBOARD";
  publisher_->publish(msg);

  // Indicate that we have successfully completed this action in one tick
  return BT::NodeStatus::SUCCESS;
}

}  // namespace nova_behavior_tree

// Register this node to make it available via XML <BlackboardPublisher/>
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::BlackboardPublisherNode>("BlackboardPublisher");
}
