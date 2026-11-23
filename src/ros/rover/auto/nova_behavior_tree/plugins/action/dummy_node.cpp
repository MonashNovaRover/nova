// Copyright (c) 2025 Monash Nova Rover
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

#include "nova_behavior_tree/action/dummy_node.hpp"

namespace nova_behavior_tree
{

    DummyNode::DummyNode(
        const std::string & name,
        const BT::NodeConfiguration & config)
        : ActionNodeBase<std_srvs::srv::Trigger>(name, config)
    {
    }

} // namespace nova_behavior_tree

BT_REGISTER_NODES(factory)
{
    factory.registerNodeType<nova_behavior_tree::DummyNode>("DUMMY_NODE");
}