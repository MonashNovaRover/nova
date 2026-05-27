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

#include "nova_behavior_tree/decorator/force_running.hpp"
#include "behaviortree_cpp/bt_factory.h"

namespace nova_behavior_tree
{

ForceRunning::ForceRunning(
  const std::string & name,
  const BT::NodeConfiguration & conf)
: BT::DecoratorNode(name, conf)
{
}

BT::NodeStatus ForceRunning::tick()
{
  if (!child()) {
    throw BT::RuntimeError("ForceRunning decorator must have a child");
  }

  child()->executeTick();
  return BT::NodeStatus::RUNNING;
}

}  // namespace nova_behavior_tree

BT_REGISTER_NODES(factory)
{
  factory.registerNodeType<nova_behavior_tree::ForceRunning>("ForceRunning");
}
