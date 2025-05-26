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

/**
 * @brief Places search goals for AR Tags and Objects related to the URC mission
 * once the rover is within the search radius.
 * 
 * @authors Terry Tian
 */

 #ifndef NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__PLACE_SEARCH_GOALS_ACTION_HPP_
 #define NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__PLACE_SEARCH_GOALS_ACTION_HPP_
 
 #include <vector>
 
 #include "geometry_msgs/msg/pose_stamped.hpp"
 #include "behaviortree_cpp/action_node.h"
 
 namespace nova_behavior_tree
 {
 
 class PlaceSearchGoalsAction : public BT::ActionNodeBase
 {
 public:
   typedef geometry_msgs::msg::PoseStamped Goal;
   typedef std::vector<Goal> Goals;
 
   PlaceSearchGoalsAction(
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
       BT::InputPort<int>("goal_type", "Goal type (nothing, AR tag, or object?)"),
       BT::InputPort<Goal>("current_pose", "The current pose of the rover"),
       BT::InputPort<double>("search_radius", 10.0, "Search radius in m"),
       BT::InputPort<double>("edge_offset", 2.5, "Offset to place goals from the edge of the search radius"),
       BT::InputPort<Goals>("input_goals", "Goals vector to add search goals into"),
       BT::OutputPort<Goals>("output_goals", "Goals with new search goals added"),
     };
   }
 
 private:
   void halt() override {}
   BT::NodeStatus tick() override;
   void place_search_goals();
 
   rclcpp::Node::SharedPtr node_;
   
   // inputs
   int goal_type_;
   double search_radius_;
   double edge_offset_;
   Goal current_pose_;
   Goals input_goals_;
   
   bool initialized_ = false;
 };
 
 }  // namespace nova_behavior_tree
 
 #endif  // NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__PLACE_SEARCH_GOALS_ACTION_HPP_