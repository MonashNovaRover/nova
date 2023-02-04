#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class gets the arm coordinate info and re-publishes
  it with message types that can be visualised in Rviz
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: arm_rviz_publisher
TOPICS:
  - /control/arm_coord_frames           [sensor_msgs/MultiDOFJointState]    [Subscribed]
  - /control/arm_control_scheme         [core/ArmControlScheme]             [Subscribed]
  - /control/control_pose               [geometry_msgs/TransformStamped]    [Subscribed]
  - /control/arm_poses                  [geometry_msgs/PoseArray]           [Published]
  - /control/arm_poses_path             [nav_msgs/Path]                     [Published]
  - /control/arm_control_pose           [geometry_msgs/PoseStamped]         [Published]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S): Jory Braun
CREATION:	 12/02/2022
EDITED:		 04/02/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS client library
#include "rclcpp/rclcpp.hpp"
// Include message types
#include "sensor_msgs/msg/multi_dof_joint_state.hpp"
#include "core/msg/arm_control_scheme.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "geometry_msgs/msg/pose_array.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav_msgs/msg/path.hpp"


class ArmVizPublisher : public rclcpp::Node
{

    //------------------------------------------------------------//
    private:
    
    // Subscriptions
    rclcpp::Subscription<sensor_msgs::msg::MultiDOFJointState>::SharedPtr coord_frames_sub;
    rclcpp::Subscription<core::msg::ArmControlScheme>::SharedPtr control_scheme_sub;
    rclcpp::Subscription<geometry_msgs::msg::TransformStamped>::SharedPtr control_pose_sub;
    
    // Publisher to /visualisation/arm_poses
    // Use to display coordinate frames
    rclcpp::Publisher<geometry_msgs::msg::PoseArray>::SharedPtr arm_poses_pub;
    
    // Publisher to /visualisation/arm_poses_path
    // Use to display lines between joints' coordinate frames
    rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr arm_path_pub;

    // Publisher to /visualisation/arm_control_pose
    // Use to display control pose when using position control
    rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr arm_control_pose_pub;

    // Store state of last-received messages
    core::msg::ArmControlScheme control_scheme;

    /// @brief  Callback for arm_coord_frames subscription
    ///         Re-publishes the position info as a PoseArray and a Path message
    void coord_frames_callback(const sensor_msgs::msg::MultiDOFJointState::SharedPtr frames_msg);

    /// @brief  Callback for arm_control_scheme subscription
    ///         Stores control scheme info for later use
    void control_scheme_callback(const core::msg::ArmControlScheme::SharedPtr msg);

    /// @brief  Callback for control_pose subscription
    ///         If using position control, re-publishes the pose as a PoseStamped message
    void control_pose_callback(const geometry_msgs::msg::TransformStamped::SharedPtr msg);
    
    //------------------------------------------------------------//
    public:

    /// Constructor. Starts the node
    ArmVizPublisher();
    
};