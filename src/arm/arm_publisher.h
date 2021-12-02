#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: 
TOPICS:
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  
CREATION:	
EDITED:		
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"
#include "core/msg/input_joystick.hpp"
#include "core/msg/arm_input.hpp"

#include <iostream>

// Use the standard namespaces
using namespace std;
using namespace std::chrono_literals;
using std::placeholders::_1;


/* 
Arm input class that handles input data from joysticks and publishes 
task and joint space velocities 
*/
class ArmPublisher : public rclcpp::Node {

    //------------------------------------------------------------//
    private:

    // Stores the loop timer for the update function
    rclcpp::TimerBase::SharedPtr timer;

    // Stores the publisher for arm inputs
    rclcpp::Publisher<core::msg::ArmInput>::SharedPtr arm_input_publisher;

    // Stores the subscribers to the joystick inputs
    rclcpp::Subscription<core::msg::InputJoystick>::SharedPtr joystick_l_subscription;
    rclcpp::Subscription<core::msg::InputJoystick>::SharedPtr joystick_r_subscription;

    // Stores task space inputs
    float task_velocity[6];

    //Stores joint space inputs
    float joint_velocity[6];

    //IK on wrist 
    bool IK_wrist = false

    //IK on lower joints
    bool IK_lower_joints = false
 
    //------------------------------------------------------------//
    private:

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_l_callback (const core::msg::InputJoystick::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_r_callback (const core::msg::InputJoystick::SharedPtr msg);

    /// @brief      Function for publishing arm input message
    void publish_arm_inputs ()

    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    ArmPublisher();
    
};