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
  - /control/endeffector_input   [core/EndEffectorInput]     [Subscribed]
  - /control/cmd_ouputs          [sensor_msgs/JointState]    [Subscribed]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Jess Hepworth, Jory Braun
CREATION:	03/12/2021
EDITED:		13/09/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS client library
#include "rclcpp/rclcpp.hpp"
// Include message types
#include "core/msg/end_effector_input.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

// Include libraries
#include "arm_config_info_client.h"
#include "cmd/cmd.h"

// Use the standard namespaces
using namespace std::chrono_literals;
using std::placeholders::_1;


/* 
Class which receives the commands for the CMDS and interfaces 
with the joint class to control the CMDs  
*/
class ArmDriver : public ArmConfigInfoClient
{
    //------------------------------------------------------------//
    private:

    // Stores the subscriber to the desired joint velocities
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_velocities_subscription;

    // Stores the subscriber to the desired actuator commands
    rclcpp::Subscription<core::msg::EndEffectorInput>::SharedPtr endeffector_input_subscription;

    // A vector of pointers to CMD instances
    std::vector<CMD*> joints;

    // A vector of cmd drive modes (mode for each joint, PWM=0, PID=1)
    std::vector<CMDCommand> CMD_drive_mode;

    // A vector of CMD directions
    std::vector<bool> CMD_direction;

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joint_velocities_callback (const sensor_msgs::msg::JointState::SharedPtr msg);
    /// @brief      Deadline callback for joint velocities subscription
    ///             Resets the internal joint velocities
    void joint_velocities_deadline_callback();
    
    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void endeffector_input_callback (const core::msg::EndEffectorInput::SharedPtr msg);
    /// @brief      Deadline callback for endeffector_inputs subscription
    ///             Resets the internal state
    void endeffector_input_deadline_callback();

    /// @brief      Application setup function. Starts publishers, subscribers and initialises members
    void start_node() override;

    //------------------------------------------------------------//
    public:

    /// @brief      Constructor. Starts the node
    ArmDriver() : ArmConfigInfoClient("arm_driver"){}
    
};
