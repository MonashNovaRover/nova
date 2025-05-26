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

#ifndef NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__LED_SERVICE_HPP_
#define NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__LED_SERVICE_HPP_

#include <string>

#include "nav2_behavior_tree/bt_service_node.hpp"
#include "std_srvs/srv/trigger.hpp"

namespace nova_behavior_tree
{

    /**
     * @brief A nav2_behavior_tree::BtServiceNode class that wraps nav2_msgs::srv::Trigger
     */
    class LedService : public nav2_behavior_tree::BtServiceNode<std_srvs::srv::Trigger>
    {
    public:
        /**
         * @brief A constructor for nova_behavior_tree::LedService
         * @param service_node_name Service name this node creates a client for
         * @param conf BT node configuration
         */
        LedService(
            const std::string &service_node_name,
            const BT::NodeConfiguration &conf);
    };

} // namespace nova_behavior_tree

#endif // NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__LED_SERVICE_HPP_