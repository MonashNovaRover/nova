#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class reads data from the arm control script and 
    publishes data to the arm CMDs. 
Whether to use PID or PWM is decided based on presence 
    of encoders on joints.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: arm_driver
TOPICS:
  - /control/arm_input      [ArmInput]                  [Subscribed]
  - /control/cmd_ouputs     [sensor_msgs/JointState]    [Subscribed]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Jess Hepworth, Jory Braun
CREATION:	03/12/2021
EDITED:		03/03/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - create cmd_outputs message
 - work out subsriber to arm parameters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"

#include "core/msg/arm_input.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

#include "joint.h"

// Use the standard namespaces
using namespace std::chrono_literals;
using std::placeholders::_1;


/* 
Class which receives the commands for the CMDS and interfaces 
with the joint class to control the CMDs  
*/
class ArmDriver : public rclcpp::Node {


    //------------------------------------------------------------//
    private:

    // Stores the loop timer for the update function
    rclcpp::TimerBase::SharedPtr timer;

    // Stores the subscriber to the desired joint velocities
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_velocities_subscription;

    // Stores the subscriber to the desired actuator commands (bypassing control script for now)
    rclcpp::Subscription<core::msg::ArmInput>::SharedPtr arm_input_subscription;

    // A vector of pointers to joint instances
    std::vector<Joint*> joints;

    // A vector of cmd drive modes (mode for each joint, PWM=0, PID=1)
    std::vector<CMDCommand> CMD_drive_mode;

    // A vector of CMD directions
    std::vector<bool> CMD_direction;
    
    /*
    * QoS options for joint velocities. These are the same as the options for the publisher, to ensure compatibility.
    *     Reliability = best effort: nodes will attempt to publish, but will not garuntee any one message is received
    *     Durability = volatile: "no attempt is made to persist samples for late joining subscribers". This is the default for the sensors QoS profile 
    *     Deadline = 500ms. If a message is not published within this time, the offered deadline callback will be called.
    */
    rclcpp::QoS subscriber_qos = rclcpp::QoS(1).reliability(RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT).durability(RMW_QOS_POLICY_DURABILITY_VOLATILE).deadline(200ms);
    // Subscription options with allocator - note, as we do not use the AllocatorT typed variables, we put void here
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> subscriber_options;

     /// @brief      Resets internal message state for joint velocities
    void zero_joints();
    
    /// @brief      Function provided to Quality of Service options 
    void deadline_callback();

    //------------------------------------------------------------//
    private:

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joint_velocities_callback (const sensor_msgs::msg::JointState::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void arm_input_callback (const core::msg::ArmInput::SharedPtr msg);

    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    ArmDriver();
    
};