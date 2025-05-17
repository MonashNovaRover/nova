#ifndef NOVA_BEHAVIOR_TREE__PLUGINS__CONDITION__AR_TAG_DETECTED_CONDITION_HPP_
#define NOVA_BEHAVIOR_TREE__PLUGINS__CONDITION__AR_TAG_DETECTED_CONDITION_HPP_

#include <aruco_opencv_msgs/msg/aruco_detection.hpp>
#include <behaviortree_cpp/basic_types.h>
#include <functional>
#include <rclcpp/callback_group.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp/subscription.hpp>
#include <string>
#include <cstdlib>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <rclcpp/rclcpp.hpp>
#include <vector>
#include "behaviortree_cpp/condition_node.h"


namespace nova_behavior_tree
{

  /**
   * @brief A BT::ConditionNode that returns SUCCESS when a goal has been detected and FAILURE otherwise
   */
  class ARTagDetectedCondition : public BT::ConditionNode
  {
  public:
    typedef std::vector<int> IDs;
    typedef std::vector<geometry_msgs::msg::PoseStamped> Goals;

    /**
     * @brief A constructor for nova_behavior_tree::ARTagDetectedCondition
     * @param condition_name Name for the XML tag for this node
     * @param conf BT node configuration
     */
    ARTagDetectedCondition(
      const std::string &condition_name,
      const BT::NodeConfiguration &conf);

    ARTagDetectedCondition() = delete;

    /**
     * @brief The main override required by a BT action
     * @return BT::NodeStatus Status of tick execution
     */
    BT::NodeStatus tick() override;

    /**
     * @brief Function to read parameters and initialize class variables
     */
    void initialize();

    /**
     * @brief Creates list of BT ports
     * @return BT::PortsList Containing node-specific ports
     */
    static BT::PortsList providedPorts()
    {
      return 
      {
        BT::OutputPort<int>("id", "ID of detected goal"),
        BT::OutputPort<geometry_msgs::msg::PoseStamped>("goal", "Pose of detected goal"),
      };
    }

  private:
    /**
     * @brief Callback to handle goal detections
     */
    void callback_ar_tag(const aruco_opencv_msgs::msg::ArucoDetection::SharedPtr msg);

    /**
     * @brief Looks for a detected goal
     * @return bool true when the goal exists, else false
     */
    bool detected();

    rclcpp::Node::SharedPtr node_;

    bool initialized_ = false;

    int goal_id_;
    std_msgs::msg::Header goal_header_;
    geometry_msgs::msg::Pose goal_pose_;
    int goal_found_ = 0;

    rclcpp::CallbackGroup::SharedPtr callback_group_;
    rclcpp::executors::MultiThreadedExecutor callback_group_executor_;
    rclcpp::Subscription<aruco_opencv_msgs::msg::ArucoDetection>::SharedPtr sub_ar_tag_;
  };

} // namespace nova_behavior_tree

#endif // NOVA_BEHAVIOR_TREE__PLUGINS__CONDITION__AR_TAG_DETECTED_CONDITION_HPP_
