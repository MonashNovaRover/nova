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

#ifndef NOVA_BEHAVIOR_TREE__PLUGINS__ACTION__REMOVE_PASSED_GOALS_AND_AR_TAGS_ACTION_HPP_
#define NOVA_BEHAVIOR_TREE__PLUGINS__ACTION__REMOVE_PASSED_GOALS_AND_AR_TAGS_ACTION_HPP_
#include <vector>
#include <memory>
#include <string>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav2_util/geometry_utils.hpp"
#include "nav2_util/robot_utils.hpp"
#include "behaviortree_cpp/action_node.h"
#include "nav2_behavior_tree/bt_utils.hpp"

namespace nova_behavior_tree
{

  class RemovePassedURCGoalsAction : public BT::ActionNodeBase
  {
  public:
    typedef std::vector<geometry_msgs::msg::PoseStamped> Goals;
    typedef std::vector<int> IDs;

    RemovePassedURCGoalsAction(
      const std::string & xml_tag_name,
      const BT::NodeConfiguration & conf);

    /**
     * @brief Function to initialize variables,
     * called only once in the lifecycle of the BT
     */
    void initialize();

    static BT::PortsList providedPorts()
    {
      return 
      {
        BT::InputPort<Goals>("input_goals", "Original goals to remove viapoints from"),
        BT::InputPort<double>("position_tolerance", 1.0, "Max distance to goal for it to be removed"),
        BT::InputPort<double>("orientation_tolerance", 0.25, "Max angle to goal for it to be removed"), // (0.25 = ~14 degrees)
        BT::OutputPort<Goals>("output_goals", "Goals with passed viapoints removed"),
        BT::OutputPort<double>("dist_to_goal", "Distance remaining to the next goal"),
      };
    }

  private:
    void halt() override {}
    BT::NodeStatus tick() override;

    rclcpp::Node::SharedPtr node_;
    std::shared_ptr<tf2_ros::Buffer> tf_;
    std::string robot_base_frame_;

    Goals input_goals_;
    double position_tolerance_;
    double orientation_tolerance_;
    double transform_tolerance_;
    bool initialized_ = false;
  };

}  // namespace nova_behavior_tree

#endif  // NOVA_BEHAVIOR_TREE__PLUGINS__ACTION__REMOVE_PASSED_GOALS_AND_AR_TAGS_ACTION_HPP_