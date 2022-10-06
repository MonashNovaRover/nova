#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class performs the task-space and joint-space control
  of the arm.
The class reads resolver data published to ROS and updates
  the arm model to match the real pose.
It reads the desired task position, task velocity and joint
  velocities and publishes the required output joint velocities
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: arm_control
TOPICS:
  - /control/arm_control_scheme        [core/ArmControlScheme]           [Subscribed]
  - /electronics/resolvers             [sensor_msgs/JointState]          [Subscribed]
  - /control/task_velocity             [geometry_msgs/TwistStamped]      [Subscribed]
  - /control/task_position             [geometry_msgs/TransformStamped]  [Subscribed]
  - /control/input_joint_velocities    [sensor_msgs/JointState]          [Subscribed]
  - /control/arm_coord_frames          [sensor_msgs/MultiDOFJointState]  [Published]
  - /control/joint_velocities          [sensor_msgs/JointState]          [Published]
SERVICES:
  - /control/arm_config_info           [core/ArmConfigInfo]             [Server]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	   control
AUTHOR(S):   Jory Braun
CREATION:	   27/09/2022
EDITED:		   01/10/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS client library
#include "rclcpp/rclcpp.hpp"
// Include message types
#include "core/msg/arm_control_scheme.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "sensor_msgs/msg/multi_dof_joint_state.hpp"
// Include service types
#include "core/srv/arm_config_info.hpp"

// Include libraries
#include "arm_model.h"
#include "arm_kinematics.h"
#include "pi_controller.h"

// Use the standard namespaces
using namespace std::chrono_literals;
using std::placeholders::_1;
using std::placeholders::_2;

/* 
Class which models the arm.
Use real positions of joints and end effectors, but idealised links
*/
class ArmControl : public rclcpp::Node
{
    //------------------------------------------------------------//
    private:

    // Subscribers
    rclcpp::Subscription<core::msg::ArmControlScheme>::SharedPtr control_scheme_sub;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr resolver_sub;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr input_joint_velocities_sub;
    rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr task_velocity_sub;
    rclcpp::Subscription<geometry_msgs::msg::TransformStamped>::SharedPtr task_position_sub;
    
    // Periods at which to publish
    std::chrono::milliseconds coord_frames_timer_period;
    std::chrono::milliseconds joint_velocities_timer_period;

    // Publisher timers
    rclcpp::TimerBase::SharedPtr coord_frames_timer;
    rclcpp::TimerBase::SharedPtr joint_velocities_timer;

    // Publishers
    rclcpp::Publisher<sensor_msgs::msg::MultiDOFJointState>::SharedPtr coord_frames_pub;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_velocities_pub;

    // Services (servers)
    rclcpp::Service<core::srv::ArmConfigInfo>::SharedPtr arm_config_info_service;

    // Store state of last-received messages
    core::msg::ArmControlScheme control_scheme;
    sensor_msgs::msg::JointState joints;
    sensor_msgs::msg::JointState joint_space_input;
    geometry_msgs::msg::TwistStamped task_velocity;
    geometry_msgs::msg::TransformStamped task_position;

    // Store messages to be published (so only need to initialise once)
    sensor_msgs::msg::MultiDOFJointState coord_frames;
    // For joint_velocities, just use velocities section in joints

    
    // Arm model and solvers
    ArmModel* arm_model;
    ArmKinematics* arm_kinematics_solver;

    // Controllers for each joint
    std::vector<PIController*> controllers;

    // Control variables
    const double ERROR_LIMIT_LINEAR = 0.2;  // m/s
    const double ERROR_LIMIT_ANGULAR = 0.7;  // rad/s
    rclcpp::Time prev_time;


    /// @brief  Callback for control scheme subscription
    ///         Updates the internal control scheme, which is used to determine how to solve IK
    void control_scheme_callback(const core::msg::ArmControlScheme::SharedPtr msg);
    
    /// @brief  Callback for resolver subscription
    ///         Updates the internal joint state, which is later used to update the model
    void resolver_callback(const sensor_msgs::msg::JointState::SharedPtr msg);

    /// @brief  Callback for input joint velocities subscription
    ///         Updates the internal joint-space joint velocities
    void input_joint_velocities_callback(const sensor_msgs::msg::JointState::SharedPtr msg);
    /// @brief  Deadline callback for input joint velocities subscription
    ///         Resets the internal joint-space joint velocities
    void input_joint_velocities_deadline_callback();

    /// @brief  Callback for task velocity subscription
    ///         Updates the internal task velocity, which is later used in the IK control loop
    void task_velocity_callback(const geometry_msgs::msg::TwistStamped::SharedPtr msg);
    /// @brief  Deadline callback for input task velocity subscription
    ///         Resets the internal task velocity
    void task_velocity_deadline_callback();

    /// @brief  Callback for task position subscription
    ///         Updates the internal task position, which is later used in the IK control loop
    void task_position_callback(const geometry_msgs::msg::TransformStamped::SharedPtr msg);
    /// @brief  Deadline callback for input task position subscription
    ///         Resets the internal task position
    void task_position_deadline_callback();
    
    /// @brief  Callback for arm_coord_frames publisher timer
    ///         Updates the arm model using the latest resolver info, publishes to arm_cord_frames
    void publish_coord_frames();

    /// @brief  Combine a 6-DOF task velocity and 6-DOF serial joint velocities, output the corresponding actual joint velocities
    KDL::JntArray combine_joint_velocities(const KDL::JntArray& joint_velocities_6dof, const KDL::Twist& task_velocity, const KDL::JntArray& joint_postions);

    /// @brief  Get the control error, which is the twist that takes the end effector pose to the
    ///         control pose in 1 second.
    KDL::Twist get_control_error(const KDL::Frame& control_pose, const KDL::Frame& end_effector_pose);

    /// @brief  Get the joint-space velocities of all joints on the arm using inverse kinematics
    ///         Uses the current joint positions and desired task velocity and position
    ///         Adds joint velocities from IK and from joint-space inputs, and implements joint limits
    KDL::JntArray get_joint_velocities(double timestep);

    /// @brief  Callback for joint_velocities publihser timer
    ///         Calculates the joint velocities from joint-space and task-space input, publishes to joint_velocities
    void publish_joint_velocities();

    /// @brief  Callback for arm model config service
    ///         Returns details of the arm model
    void arm_config_info_callback(
        const core::srv::ArmConfigInfo::Request::SharedPtr request,
        core::srv::ArmConfigInfo::Response::SharedPtr response
    );
    
    //------------------------------------------------------------//
    public:

    /// Constructor. Starts publishers, subscribers and initialises members
    ArmControl();
    
};
