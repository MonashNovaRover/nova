#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class reads data from the arm control script and 
    publishes data to the arm CMDs. 

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
EDITED:		02/10/2022
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
#include "arm_model.h"


/* 
Class which receives the commands for the CMDs and drives the joints
*/
class ArmDriver : public rclcpp::Node
{
    //------------------------------------------------------------//
    private:

    // Arm model. Includes motor drivers for all joints
    ArmModel* arm_model;
    // End effector
    CMD* end_effector;

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

    //------------------------------------------------------------//
    public:

    /// @brief      Constructor. Starts publishers, subscribers and initialises members
    ArmDriver();

};
