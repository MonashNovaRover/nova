// Copyright (c) 2026 Monash Nova Rover
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

#ifndef NOVA_BEHAVIOR_TREE__PLUGINS__CONDITION__TOPICS_ACTIVE_CONDITION_HPP_
#define NOVA_BEHAVIOR_TREE__PLUGINS__CONDITION__TOPICS_ACTIVE_CONDITION_HPP_

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/generic_subscription.hpp"
#include "rclcpp/serialized_message.hpp"
#include "behaviortree_cpp/condition_node.h"

namespace nova_behavior_tree
{

class TopicsActiveCondition : public BT::ConditionNode
{
public:
  TopicsActiveCondition(
    const std::string & condition_name,
    const BT::NodeConfiguration & conf);

  ~TopicsActiveCondition() override = default;

  BT::NodeStatus tick() override;

  static BT::PortsList providedPorts()
  {
    return {
      BT::InputPort<std::vector<std::string>>("topics", "Topics to inspect"),
      BT::InputPort<std::vector<double>>("active_ages", "Time in seconds since last received message to consider active for each topic"),
      BT::OutputPort<std::vector<bool>>("active_states", "Per-topic active state")
    };
  }

private:
  void initialize();

  rclcpp::Node::SharedPtr node_;
  rclcpp::CallbackGroup::SharedPtr callback_group_;
  rclcpp::executors::MultiThreadedExecutor callback_group_executor_;
  std::vector<rclcpp::GenericSubscription::SharedPtr> subscriptions_;
  std::unordered_map<std::string, rclcpp::Time> last_message_time_;
  std::vector<std::string> topics_;
  std::vector<double> active_ages_;
  bool initialized_ = false;
  std::mutex mutex_;
};

}  // namespace nova_behavior_tree

#endif  // NOVA_BEHAVIOR_TREE__PLUGINS__CONDITION__TOPICS_ACTIVE_CONDITION_HPP_
