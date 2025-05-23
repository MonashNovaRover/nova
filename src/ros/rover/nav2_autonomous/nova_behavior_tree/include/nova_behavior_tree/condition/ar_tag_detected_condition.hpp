#ifndef NOVA_BEHAVIOR_TREE__PLUGINS__CONDITION__AR_TAG_DETECTED_CONDITION_HPP_
#define NOVA_BEHAVIOR_TREE__PLUGINS__CONDITION__AR_TAG_DETECTED_CONDITION_HPP_

#include <string>
#include <vector>
#include <cstdlib>
#include <functional>
#include <queue>

#include <aruco_opencv_msgs/msg/aruco_detection.hpp>
#include <behaviortree_cpp/basic_types.h>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp/callback_group.hpp>
#include <rclcpp/executors/multi_threaded_executor.hpp>
#include <rclcpp/subscription.hpp>

#include "behaviortree_cpp/condition_node.h"


namespace nova_behavior_tree
{

  /**
   * @brief A BT::ConditionNode that returns SUCCESS when a goal has been detected and FAILURE otherwise
   */
  class ARTagDetectedCondition : public BT::ConditionNode
  {
  public:
    typedef geometry_msgs::msg::PoseStamped Goal;

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
        BT::InputPort<int>("min_detections", 2, "Minimum number of detections within buffer time to be considered valid"),
        BT::InputPort<double>("buffer_time", 5.0, "Keep track of detections in the last n seconds"),
        BT::OutputPort<Goal>("goal", "Detected AR tag pose"),
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
    rclcpp::CallbackGroup::SharedPtr callback_group_;
    rclcpp::executors::MultiThreadedExecutor callback_group_executor_;
    rclcpp::Subscription<aruco_opencv_msgs::msg::ArucoDetection>::SharedPtr sub_ar_tag_;

    int goal_id_;
    Goal goal_;
    bool goal_found_ = false;
    int min_detections_;
    double buffer_time_;
    std::queue<Goal> detections_buffer_;
    
    bool initialized_ = false;
  };

} // namespace nova_behavior_tree

#endif // NOVA_BEHAVIOR_TREE__PLUGINS__CONDITION__AR_TAG_DETECTED_CONDITION_HPP_
