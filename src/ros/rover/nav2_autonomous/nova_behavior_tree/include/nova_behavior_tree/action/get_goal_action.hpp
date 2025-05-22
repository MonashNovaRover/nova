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

/**
 * @brief Extracts a goal from a goals vector based on the given index.
 * 
 * @authors Terry Tian
 */

 #ifndef NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__GET_GOAL_ACTION_HPP_
 #define NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__GET_GOAL_ACTION_HPP_
 
 #include <string>
 #include <vector>
 
 #include "geometry_msgs/msg/pose_stamped.hpp"
 #include "rclcpp/rclcpp.hpp"
 
 #include "behaviortree_cpp/action_node.h"
 
 namespace nova_behavior_tree
 {
 
 class GetGoalAction : public BT::ActionNodeBase
 {
 public:
   typedef geometry_msgs::msg::PoseStamped Goal;
   typedef std::vector<Goal> Goals;
 
   GetGoalAction(
     const std::string & xml_tag_name,
     const BT::NodeConfiguration & conf);
 
   /**
    * @brief Function to initialize variables,
    * called only once in the lifecycle of the BT
    */
   void initialize();
 
   static BT::PortsList providedPorts()
   {
     return {
       BT::InputPort<int>("index", "Index of the goal to extract (negative indexing is supported)"),
       BT::InputPort<Goals>("input_goals", "Goals vector to extract goal from"),
       BT::OutputPort<Goal>("output_goal", "Extracted goal"),
     };
   }
 
 private:
   void halt() override {}
   BT::NodeStatus tick() override;
   bool get_goal();
 
   rclcpp::Node::SharedPtr node_;
   
   // inputs
   int index_;
   Goals input_goals_;
   
   bool initialized_ = false;
 };
 
 }  // namespace nova_behavior_tree
 
 #endif  // NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__GET_GOAL_ACTION_HPP_