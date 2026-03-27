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

#include "nova_behavior_tree/condition/topics_active_condition.hpp"
#include "rclcpp/logging.hpp"

namespace nova_behavior_tree
{

TopicsActiveCondition::TopicsActiveCondition(
  const std::string & condition_name,
  const BT::NodeConfiguration & conf)
: BT::ConditionNode(condition_name, conf)
{
}

void TopicsActiveCondition::initialize()
{
  node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");

  if (!getInput("topics", topics_)) {
    RCLCPP_ERROR(node_->get_logger(), "TopicsActiveCondition requires 'topics' input port");
    topics_.clear();
  }

  if (!getInput("active_ages", active_ages_)) {
    RCLCPP_WARN(node_->get_logger(), "TopicsActiveCondition using default active_ages (0.5 for each topic)");
    active_ages_ = std::vector<double>(topics_.size(), 0.5);
  } else if (active_ages_.size() != topics_.size()) {
    RCLCPP_ERROR(node_->get_logger(), "TopicsActiveCondition: active_ages size (%zu) must match topics size (%zu)", 
                 active_ages_.size(), topics_.size());
    active_ages_ = std::vector<double>(topics_.size(), 0.5);
  }

  callback_group_ = node_->create_callback_group(
    rclcpp::CallbackGroupType::MutuallyExclusive,
    false
  );
  callback_group_executor_.add_callback_group(callback_group_, node_->get_node_base_interface());

  rclcpp::SubscriptionOptions subopts;
  subopts.callback_group = callback_group_;

  auto known_topics = node_->get_topic_names_and_types();

  for (const auto &topic : topics_) {
    auto it = known_topics.find(topic);
    if (it == known_topics.end()) {
      RCLCPP_WARN(node_->get_logger(), "Topic '%s' not found and cannot be monitored", topic.c_str());
      continue;
    }

    if (it->second.empty()) {
      RCLCPP_WARN(node_->get_logger(), "No type information for topic '%s'", topic.c_str());
      continue;
    }

    auto topic_type = it->second.front();

    try {
      auto sub = node_->create_generic_subscription(
        topic,
        topic_type,
        rclcpp::QoS(1),
        [this, topic](std::shared_ptr<rclcpp::SerializedMessage> /*msg*/) {
          std::lock_guard<std::mutex> lock(mutex_);
          last_message_time_[topic] = node_->now();
        },
        subopts);

      subscriptions_.push_back(sub);
      RCLCPP_INFO(node_->get_logger(), "Monitoring topic '%s' (type '%s')", topic.c_str(), topic_type.c_str());
    } catch (const std::exception & e) {
      RCLCPP_ERROR(node_->get_logger(), "Failed to create generic subscription for '%s': %s", topic.c_str(), e.what());
    }
  }

  initialized_ = true;
}

BT::NodeStatus TopicsActiveCondition::tick()
{
  if (!initialized_) {
    initialize();
  }

  callback_group_executor_.spin_some();

  auto now = node_->now();
  std::vector<bool> active_states;
  bool all_active = true;

  {
    std::lock_guard<std::mutex> lock(mutex_);

    for (size_t i = 0; i < topics_.size(); ++i) {
      const auto &topic = topics_[i];
      double active_age = (i < active_ages_.size()) ? active_ages_[i] : 1.0;
      
      bool active = false;
      auto it = last_message_time_.find(topic);
      if (it != last_message_time_.end()) {
        auto age = (now - it->second).seconds();
        if (age <= active_age) {
          active = true;
        }
        else {
            RCLCPP_WARN(node_->get_logger(), "Topic '%s' last message age %.2f exceeds active age %.2f!", topic.c_str(), age, active_age);
        }
      }
      active_states.push_back(active);
      if (!active) {
        all_active = false;
      }
    }
  }

  setOutput("active_states", active_states);

  if (all_active && !topics_.empty()) {
    return BT::NodeStatus::SUCCESS;
  }

  return BT::NodeStatus::FAILURE;
}

}  // namespace nova_behavior_tree

#include "behaviortree_cpp/bt_factory.h"
BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::TopicsActiveCondition>("TopicsActive");
}
