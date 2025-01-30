#ifndef NOVA_BEHAVIOR_TREE__BLACKBOARD_PUBLISHER_NODE_HPP_
#define NOVA_BEHAVIOR_TREE__BLACKBOARD_PUBLISHER_NODE_HPP_

#include <string>
#include <memory>
#include "behaviortree_cpp/action_node.h"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

namespace nova_behavior_tree
{

class BlackboardPublisherNode : public BT::ActionNodeBase
{
public:
  BlackboardPublisherNode(
    const std::string & xml_tag_name,
    const BT::NodeConfiguration & conf);

  static BT::PortsList providedPorts()
  {
    return {
      // Optional: Add input ports if needed (e.g., topic name)
      BT::InputPort<std::string>("topic_name", "blackboard_data", "ROS topic for publishing"),
    };
  }

  void initialize();
  void halt() override {}  // No cleanup needed
  BT::NodeStatus tick() override;

private:
  rclcpp::Node::SharedPtr node_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  std::string topic_name_;
};

}  // namespace nova_behavior_tree

#endif  // NOVA_BEHAVIOR_TREE__BLACKBOARD_PUBLISHER_NODE_HPP_