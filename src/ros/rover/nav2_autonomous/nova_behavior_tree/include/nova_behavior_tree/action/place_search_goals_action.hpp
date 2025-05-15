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
 * @brief Places search goals for AR Tags and Objects related to the URC mission.
 * Once the rover enters the search radius, four search goals will be placed, drawing out
 * the shape of an equilateral triangle.
 * 
 * If you imagine the search origin where 0 degrees represents the direction the rover is facing
 * (so the rover's entry point is 180 degrees from the search origin), the search goals are inserted
 * on the edge of the search radius as follows:
 * 1. 0 degrees
 * 2. 120 degrees (right)
 * 3. -120 degrees (left)
 * 4. 0 degrees
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
       BT::InputPort<Goals>("input_goals", "Goals vector to add search goals into"),
       BT::OutputPort<Goals>("output_goals", "Goals with new search goals added"),
     };
   }
 
 private:
   void halt() override {}
   BT::NodeStatus tick() override;
   void place_search_goals();
 
   rclcpp::Node::SharedPtr node_;
   
   int goal_type;
   Goal current_pose_;
   Goals input_goals_;
   
   bool initialized_ = false;

   // search radii of goals as per the URC rulebook
   static constexpr int AR_TAG_SEARCH_RADIUS = 20;
   static constexpr int OBJECT_SEARCH_RADIUS = 10;
 };
 
 }  // namespace nova_behavior_tree
 
 #endif  // NAV2_BEHAVIOR_TREE__PLUGINS__ACTION__PLACE_SEARCH_GOALS_ACTION_HPP_