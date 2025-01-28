// Copyright (c) 2021 Samsung Research America
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

#ifndef NAV2_BEHAVIOR_TREE__BASE__ACTION__EMIT_DATA_ACTION_BASE_HPP_
#define NAV2_BEHAVIOR_TREE__BASE__ACTION__EMIT_DATA_ACTION_BASE_HPP_

#include <string>
#include <algorithm>
#include <random>
#include <chrono>

#include "behaviortree_cpp/action_node.h"
#include "rclcpp/logging.hpp"

namespace nova_behavior_tree
{

    using namespace std::chrono;

    /**
     * @brief A base class for emitting (dummy) data at a specified rate
     * @param T The type of data to emit
     */
    template <typename T>
    class EmitDataActionBase : public BT::StatefulActionNode
    {
    public:
        EmitDataActionBase(
            const std::string &xml_tag_name,
            const BT::NodeConfiguration &conf)
            : BT::StatefulActionNode(xml_tag_name, conf)
        {
        }

        /**
         * @brief Function to read parameters and initialize class variables
         */
        void initialize()
        {
            getInput("delay_min_s", delay_min_);
            getInput("delay_max_s", delay_max_);
            getInput("repeat_limit", repeats_left_);
            getInput("output_key", output_key_);

            // Ensure valid delay values
            delay_min_ = std::max(0.0, delay_min_);
            delay_max_ = std::max(delay_min_, delay_max_);

            // Initialize random number generator
            unif = std::uniform_real_distribution<double>(delay_min_, delay_max_);

            delay_ = unif(re);
            node_ = config().blackboard->get<rclcpp::Node::SharedPtr>("node");
        }

        static BT::PortsList providedPorts()
        {
            return {
                BT::InputPort<double>("delay_min_s", 0.0, "Minimum delay in seconds between data emissions"),
                BT::InputPort<double>("delay_max_s", 0.0, "Maximum delay in seconds between data emissions"),
                BT::InputPort<int>("repeat_limit", 0, "Number of times to emit data, set to -1 for infinite"),
                BT::InputPort<std::string>("output_key", "Key to write the data to"),
            };
        }

        NodeStatus onStart() override
        {
            initialize();

            if (delay_ == 0.0)
            {
                setOutput(output_key_, generateData());

                if (repeats_left_-- == 0)
                {
                    return NodeStatus::SUCCESS;
                }
            }

            deadline_ = system_clock::now() + duration<double>(delay_);
            return NodeStatus::RUNNING;
        }

        NodeStatus onRunning() override
        {
            if (system_clock::now() >= deadline_)
            {
                setOutput(output_key_, generateData());

                if (repeats_left_-- == 0)
                {
                    return NodeStatus::SUCCESS;
                }

                delay_ = unif(re);
                deadline_ = system_clock::now() + duration<double>(delay_);
            }

            return NodeStatus::RUNNING;
        }

        void onHalted() override
        {
            int repeat_limit;
            getInput("repeat_limit", repeat_limit);
            RCLCPP_INFO(node_->get_logger(), "Dummy data emitter node %s halted after repeating %d time(s)", name().c_str(), repeat_limit - repeats_left_);
        }

    protected:
        virtual T generateData() = 0;
        rclcpp::Node::SharedPtr node_;

    private:
        double delay_min_;
        double delay_max_;
        int repeats_left_;
        std::string output_key_;

        double delay_;
        std::uniform_real_distribution<double> unif;
        std::default_random_engine re;
        system_clock::time_point deadline_;
    };

} // namespace nova_behavior_tree

#endif // NAV2_BEHAVIOR_TREE__BASE__ACTION__EMIT_DATA_ACTION_BASE_HPP_