#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class transforms the task-space twist from the 
  joysticks to the rover frame
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: arm_twistmapper
TOPICS:
  - /control/arm_control_scheme        [core/ArmControlScheme]           [Subscribed]
  - /electronics/resolvers             [sensor_msgs/JointState]          [Subscribed]
  - /control/input_task_velocity       [geometry_msgs/TwistStamped]      [Subscribed]
  - /control/task_velocity             [geometry_msgs/TwistStamped]      [Published]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S):   Jory Braun
CREATION:	 25/09/2022
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

// Include libraries
#include "arm_model.h"
#include "spm_kinematics.h"
#include <kdl/treefksolverpos_recursive.hpp>

// Use the standard namespaces
using namespace std::chrono_literals;
using std::placeholders::_1;

/*
Class which transforms the input twist to the rover coordinate frame
*/
class ArmTwistMapper : public rclcpp::Node
{
    //------------------------------------------------------------//
    private:

    // Store the subscribers
    rclcpp::Subscription<core::msg::ArmControlScheme>::SharedPtr control_scheme_sub;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr resolver_sub;
    rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr input_task_velocity_sub;
    
    // Store the loop timers for publishing
    std::chrono::milliseconds task_velocity_timer_period;
    rclcpp::TimerBase::SharedPtr task_velocity_timer;

    // Store the publishers
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr task_velocity_pub;

    // Store state of last-received messages
    core::msg::ArmControlScheme control_scheme;
    sensor_msgs::msg::JointState joints;
    geometry_msgs::msg::TwistStamped input_task_velocity;
    
    // Stores messages to be published
    geometry_msgs::msg::TwistStamped task_velocity;

    // Arm model and solvers
    ArmModel* arm_model;
    KDL::TreeFkSolverPos_recursive* serial_fk_solver;
    SpmKinematics* spm_solver;

    /// @brief  Callback for control scheme subscription
    ///         Updates the internal control scheme, which is used to determine how to solve IK
    void control_scheme_callback(const core::msg::ArmControlScheme::SharedPtr msg);

    /// @brief  Callback for resolver subscription
    ///         Updates the internal joint state, which is later used to update the model
    void resolver_callback(const sensor_msgs::msg::JointState::SharedPtr msg);

    /// @brief  Callback for task velocity subscription
    ///         Updates the internal task velocity, which is later used to calculate the inverse kinematics
    void input_task_velocity_callback(const geometry_msgs::msg::TwistStamped::SharedPtr msg);
    /// @brief  Deadline callback for input task velocity subscription
    ///         Resets the internal task velocity
    void input_task_velocity_deadline_callback();

    /// @brief  Get the joint-space positions of the serial model of the arm
    ///         Return as a JntArray for use with KDL kinematics solvers
    KDL::JntArray get_serial_joint_positions();

    /// @brief  Calculate the FK for a single segment using the serial model of the arm
    KDL::Frame calculate_serial_fk(KDL::JntArray kdl_joints, std::string segment_name);

    /// @brief  Get the base-frame twist given the current input twist and the selected control scheme
    KDL::Twist get_control_twist();

    /// @brief  Callback for task_velocity publihser timer
    ///         Calculates the rover-frame control twist from the task-space input, publishes to task_velocity
    void publish_task_velocity();
    
    //------------------------------------------------------------//
    public:

    /// Constructor. Initialisers the solvers and starts the node
    ArmTwistMapper();
    
};