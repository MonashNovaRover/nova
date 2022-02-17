#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class performs the forward and inverse kinematics
  calculations for the arm.
The class reads resolver data published to ROS and updates
  the arm model to match the real pose.
It reads the current task velocity and IK parameters
  published by the input node and publishes the required
  joint velocities.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: arm_kinematics
TOPICS:
  - /electronics/resolvers     [sensor_msgs/JointState]          [Subscribed]
  - /control/task_velocity     [geometry_msgs/TwistStamped]      [Subscribed]
  - /control/arm_coord_frames  [sensor_msgs/MultiDOFJointState]  [Published]
  - /control/joint_velocities  [sensor_msgs/JointState]          [Published]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S): Jory Braun
CREATION:	 11/12/2021
EDITED:		 22/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Get rid of hard-coding of arm structure
 - Change how end effector angles are specified. Use Euler ZYX instead of XYZ.
 - Try initialising message types using a different constructor.
 - Revisit FK data types wrangling
 - Implement IK
 - Publish on timer or publish on change in state? Make consistent.
   On timer prevents slow computations from holding up new messages being received.
   Keeps tasks independent.
 - Item Two
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS client library
#include "rclcpp/rclcpp.hpp"
// Include message types
#include "sensor_msgs/msg/joint_state.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "sensor_msgs/msg/multi_dof_joint_state.hpp"

// Include libraries
#include "arm_model.h"
#include <kdl/treefksolverpos_recursive.hpp>
#include <kdl/treeiksolvervel_wdls.hpp>

// Use the standard namespaces
// For publishers
using namespace std::chrono_literals;
// For subscribers
using std::placeholders::_1;

/* 
Class which models the arm.
Use real positions of joints and end effectors, but idealised links
*/
class ArmKinematics : public rclcpp::Node
{

    //------------------------------------------------------------//
    private:

    // Periods at which to publish
    std::chrono::milliseconds coord_frames_timer_period;
    std::chrono::milliseconds joint_velocities_timer_period;

    // Track internal state
    // Resolvers and joint velocities
    sensor_msgs::msg::JointState joints;
    // Task velocity
    geometry_msgs::msg::TwistStamped task_velocity;
    
    // Store other output messages so only need to initialise constant info once
    sensor_msgs::msg::MultiDOFJointState coord_frames;

    // Arm model and solvers
    ArmModel arm_model;
    // FK solver using KDL
    KDL::TreeFkSolverPos_recursive* arm_fk_solver;
    // IK solver using KDL
    KDL::TreeIkSolverVel_wdls* arm_ik_solver;

    // Subscription to resolvers
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr resolver_sub;
    // Subscription to task velocity
    rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr task_velocity_sub;
    // Publisher to /control/arm_coord_frames
    rclcpp::TimerBase::SharedPtr coord_frames_timer;
    rclcpp::Publisher<sensor_msgs::msg::MultiDOFJointState>::SharedPtr coord_frames_pub;
    // Publisher to /control/joint_velocities
    rclcpp::TimerBase::SharedPtr joint_velocities_timer;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_velocities_pub;

    /// @brief  Callback for resolver subscription
    ///         Updates the internal joint state, which is later used to update the model
    void resolver_callback(const sensor_msgs::msg::JointState::SharedPtr msg);

    /// @brief  Callback for task velocity subscription
    ///         Updates the internal velocity, which is later used to calculate the inverse kinematics
    void task_velocity_callback(const geometry_msgs::msg::TwistStamped::SharedPtr msg);

    /// @brief  Callback for arm_coord_frames publisher timer
    ///         Updates the arm model using the latest resolver info, publishes to arm_cord_frames
    void publish_coord_frames();

    /// @brief  Callback for joint_velocities publihser timer
    ///         Calculates the inverse kinematics using the latest arm model, publishes to joint_velocities
    void publish_joint_velocities();
    
    //------------------------------------------------------------//
    public:

    /// Constructor. Initialisers the solvers and starts the node
    ArmKinematics();
    
};