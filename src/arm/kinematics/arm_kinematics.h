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
  - /control/arm_control_scheme        [core/ArmControlScheme]           [Subscribed]
  - /electronics/resolvers             [sensor_msgs/JointState]          [Subscribed]
  - /control/task_velocity             [geometry_msgs/TwistStamped]      [Subscribed]
  - /control/input_joint_velocities    [sensor_msgs/JointState]          [Subscribed]
  - /control/arm_coord_frames          [sensor_msgs/MultiDOFJointState]  [Published]
  - /control/joint_velocities          [sensor_msgs/JointState]          [Published]
SERVICES:
  - /control/arm_config_info           [core/ArmConfigInfo]             [Server]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S): Jory Braun
CREATION:	 11/12/2021
EDITED:		 25/09/2022
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
#include "sensor_msgs/msg/multi_dof_joint_state.hpp"
// Include service types
#include "core/srv/arm_config_info.hpp"

// Include libraries
#include "arm_model.h"
#include "spm_kinematics.h"
#include <kdl/treefksolverpos_recursive.hpp>
#include <kdl/treeiksolvervel_wdls.hpp>

// Use the standard namespaces
using namespace std::chrono_literals;
using std::placeholders::_1;
using std::placeholders::_2;

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
    // Arm control scheme
    core::msg::ArmControlScheme control_scheme;
    // Resolvers and output joint velocities
    sensor_msgs::msg::JointState joints;
    // Joint velocities from joints-space input
    sensor_msgs::msg::JointState joint_space_input;
    // Task velocity
    geometry_msgs::msg::TwistStamped task_velocity;
    
    // Store other output messages so only need to initialise constant info once
    sensor_msgs::msg::MultiDOFJointState coord_frames;

    // Arm model and solvers
    ArmModel* arm_model;
    // Serial FK solver using KDL
    KDL::TreeFkSolverPos_recursive* serial_fk_solver;
    // Serial IK solver using KDL
    KDL::TreeIkSolverVel_wdls* serial_ik_solver;
    // SPM kinematics solver
    SpmKinematics* spm_solver;

    // Subscription to arm control scheme
    rclcpp::Subscription<core::msg::ArmControlScheme>::SharedPtr control_scheme_sub;
    // Subscription to resolvers
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr resolver_sub;
    // Subscription to input joint velocities
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr input_joint_velocities_sub;
    // Subscription to task velocity
    rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr task_velocity_sub;
    // Publisher to /control/arm_coord_frames
    rclcpp::TimerBase::SharedPtr coord_frames_timer;
    rclcpp::Publisher<sensor_msgs::msg::MultiDOFJointState>::SharedPtr coord_frames_pub;
    // Publisher to /control/joint_velocities
    rclcpp::TimerBase::SharedPtr joint_velocities_timer;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_velocities_pub;

    // Service for /control/arm_config_info
    rclcpp::Service<core::srv::ArmConfigInfo>::SharedPtr arm_config_info_service;

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
    ///         Updates the internal task velocity, which is later used to calculate the inverse kinematics
    void task_velocity_callback(const geometry_msgs::msg::TwistStamped::SharedPtr msg);
    /// @brief  Deadline callback for input task velocity subscription
    ///         Resets the internal task velocity
    void task_velocity_deadline_callback();

    /// @brief  Get the joint-space positions of the serial model of the arm
    ///         Return as a JntArray for use with KDL kinematics solvers
    KDL::JntArray get_serial_joint_positions();

    /// @brief  Calculate the FK for a single segment using the serial model of the arm
    KDL::Frame calculate_serial_fk(KDL::JntArray kdl_joints, std::string segment_name);
    
    /// @brief  Get the task-space positions of all coordinate frames on the arm using forward kinematics
    void update_coord_frames();
    
    /// @brief  Callback for arm_coord_frames publisher timer
    ///         Updates the arm model using the latest resolver info, publishes to arm_cord_frames
    void publish_coord_frames();

    /// @brief  Calculate the IK for the end effector using the serial model of the arm
    KDL::JntArray calculate_serial_ik(KDL::JntArray kdl_joint_positions, KDL::Twist kdl_twist);
    
    /// @brief  Get the joint-space velocities of all joints on the arm using inverse kinematics
    ///         Uses the current joint positions and desired task velocity
    ///         Adds joint velocities from IK and from joint-space inputs, and implements joint limits
    void update_joint_velocities();

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

    /// Constructor. Initialisers the solvers and starts the node
    ArmKinematics();
    
};