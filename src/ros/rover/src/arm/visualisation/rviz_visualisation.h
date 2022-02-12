#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class gets the arm coordinate info and re-publishes
  it with message types that can be visualised in Rviz
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: rviz_visualisation
TOPICS:
  - /control/arm_coord_frames   [sensor_msgs/MultiDOFJointState]    [Subscribed]
  - /control/arm_poses          [geometry_msgs/PoseArray]           [Published]
  - /control/arm_poses_path     [nav_msgs/Path]                     [Published]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S): Jory Braun
CREATION:	 12/02/2022
EDITED:		 12/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS client library
#include "rclcpp/rclcpp.hpp"
// Include message types
#include "sensor_msgs/msg/multi_dof_joint_state.hpp"
#include "geometry_msgs/msg/pose_array.hpp"
#include "nav_msgs/msg/path.hpp"

// Use the standard namespaces
// For publishers
using namespace std::chrono_literals;
// For subscribers
using std::placeholders::_1;


class RvizVisualisation : public rclcpp::Node
{

    //------------------------------------------------------------//
    private:
    
    // Subscription to /control/arm_coord_frames
    rclcpp::Subscription<sensor_msgs::msg::MultiDOFJointState>::SharedPtr coord_frames_sub;
    
    // Publisher to /control/arm_poses
    // Use to display coordinate frames in Rviz
    rclcpp::Publisher<geometry_msgs::msg::PoseArray>::SharedPtr arm_poses_pub;
    
    // Publisher to /control/arm_poses_path
    // Use to display lines between coordinate frames in Rviz
    rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr arm_path_pub;

    /// @brief  Callback for arm_coord_frames subscription
    ///         Re-publishes the position info as a PoseArray and a Path message
    void coord_frames_callback(const sensor_msgs::msg::MultiDOFJointState::SharedPtr frames_msg);
    
    //------------------------------------------------------------//
    public:

    /// Constructor. Starts the node
    RvizVisualisation();
    
};