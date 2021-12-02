#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class reads data from the raw joystick inputs
    and converts them to arm input messages.
This does not interface with the CMD library, but
    instead can be run on the base station to send
    arm data across the network.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: arm_pub
TOPICS:
  - /control/input_joystick_l   [InputJoystick]     [Subscribed]
  - /control/input_joystick_r   [InputJoystick]     [Subscribed]
  - /control/arm_input          [ArmInput]          [Published]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Jess Hepworth
CREATION:	02/12/2021
EDITED:		03/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Add in additional inputs for linear actuate
 - Test with CMD code with subscriber
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"
#include "core/msg/input_joystick.hpp"
#include "core/msg/arm_input.hpp"

// Use the standard namespaces
using namespace std::chrono_literals;
using std::placeholders::_1;

// Define constants
#define NUM_JOINTS 6    // Number of joints in the arm

/* 
Arm input class that handles input data from joysticks and publishes 
task and joint space velocities 
*/
class ArmPublisher : public rclcpp::Node {

    //------------------------------------------------------------//
    private:

    // Stores the loop timer for the update function
    rclcpp::TimerBase::SharedPtr timer;

    // Stores a counter for each step
    size_t count;

    // Stores the publisher for arm inputs
    rclcpp::Publisher<core::msg::ArmInput>::SharedPtr arm_publisher;

    // Stores the subscribers to the joystick inputs
    rclcpp::Subscription<core::msg::InputJoystick>::SharedPtr joystick_l_subscription;
    rclcpp::Subscription<core::msg::InputJoystick>::SharedPtr joystick_r_subscription;

    // Stores task space inputs
    float task_velocity[NUM_JOINTS];

    // Stores joint space inputs
    float joint_velocity[NUM_JOINTS];

    // flag for IK on wrist 
    bool IK_wrist = false;

    // flag for IK on lower joints
    bool IK_lower_joints = false;
 

    //------------------------------------------------------------//
    private:

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_l_callback (const core::msg::InputJoystick::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_r_callback (const core::msg::InputJoystick::SharedPtr msg);

    /// @brief      Function for publishing arm input message
    void publish_arm_inputs ();


    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    ArmPublisher();
    
};