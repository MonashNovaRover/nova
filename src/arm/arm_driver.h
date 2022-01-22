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
  - /control/arm_input      [ArmInput]  [Subscribed]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Jess Hepworth
CREATION:	03/12/2021
EDITED:		18/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - create cmd_outputs message
 - work out subsriber to arm parameters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"
// #include "core/msg/cmd_outputs.hpp"
#include "core/msg/arm_input.hpp"

#include "joint.h"

// Get shared arm info
#include "arm_core.h"

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

    // Stores a counter for each step
    size_t count;

    // Stores the subscriber to the desired joint commands
    // rclcpp::Subscription<core::msg::cmd_outputs>::SharedPtr cmd_outputs_subscription;

    // Stores the subscriber to the arm parameters
    // rclcpp::Subscription<core::msg::arm_params>::SharedPtr arm_params_subscription;

    // Stores the subscriber to the desired joint commands (bypassing control script for now)
    rclcpp::Subscription<core::msg::ArmInput>::SharedPtr arm_input_subscription;

    // An array of joint instances
    Joint* joints[NUM_JOINTS + 1];

    // An array of cmd drive modes (mode for each joint, PWM=0, PID=1)
    // Seventh 'joint' is end effector actuation
    CMDCommand CMD_drive_mode[NUM_JOINTS + 1] = {PID, PID, PID, PWM, PWM, PWM, PWM};

    //------------------------------------------------------------//
    private:

    // /// @brief      Callback function when input messages are received.
    // /// @param      msg - A pointer to the input message
    // void cmd_outputs_callback (const core::msg::CMDOutput::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void arm_input_callback (const core::msg::ArmInput::SharedPtr msg);

    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    ArmDriver();
    
};