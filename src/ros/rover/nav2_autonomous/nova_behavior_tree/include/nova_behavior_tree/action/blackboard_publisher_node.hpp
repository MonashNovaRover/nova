#ifndef NOVA_BEHAVIOR_TREE__BLACKBOARD_PUBLISHER_NODE_HPP_
#define NOVA_BEHAVIOR_TREE__BLACKBOARD_PUBLISHER_NODE_HPP_

#include <string>
#include <vector>
#include "behaviortree_cpp/action_node.h"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

namespace nova_behavior_tree
{

class BlackboardPublisherNode : public BT::ActionNodeBase
{
public:
  BlackboardPublisherNode(
    const std::string & name,
    const BT::NodeConfiguration & config);

  // Define ports if you want to read them from XML.
  // For a simple example, we include "topic_name" so you can override it in the XML.
  static BT::PortsList providedPorts()
  {
    return {
      BT::InputPort<std::string>("topic_name", "blackboard", "ROS topic for publishing"),
      BT::InputPort<std::vector<std::string>>("keys", "Blackboard keys to publish"),
      BT::InputPort<double>("publish_frequency", 1, "Publish rate in hz")
    };
  }

  void halt() override {}  // No cleanup logic needed for this example
  BT::NodeStatus tick() override;

private:
  // Called once at the start of a tick, to ensure everything is ready
  void initialize();
  void publish_blackboard();
  void split_key_string();

  rclcpp::Node::SharedPtr node_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  std::string topic_name_;
  double publish_delay_;
  std::shared_ptr<BT::Blackboard> bb_;
  std::string keys_string_;
  std::vector<std::string> keys_;
  std::chrono::steady_clock::time_point last_publish_;

  bool initialized_ = false;
};

}  // namespace nova_behavior_tree

#endif  // NOVA_BEHAVIOR_TREE__BLACKBOARD_PUBLISHER_NODE_HPP_