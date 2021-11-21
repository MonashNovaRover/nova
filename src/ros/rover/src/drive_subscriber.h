#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class interfaces with the wheels and ROS and
    is able to publish data over CAN to the wheels.
The wheels are indexed with 0, 1, 2 on the left and
    3, 4, 5 on the right, with the largest at the back.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: drive_sub
TOPICS:
  - /control/input_gamepad  [InputGamepad]  [Subscribed]
  - /control/drive_cmds     [DriveCmd]      [Subscribed]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Harrison Verrios
CREATION:	21/11/2021
EDITED:		21/11/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Test with the CAN classes
 - Test with real rover driving
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"
#include "core/msg/input_gamepad.hpp"
#include "core/msg/drive_cmd.hpp"

// Include wheel class
#include "wheel.h"

// Use the standard namespaces
using namespace std;
using namespace std::chrono_literals;
using std::placeholders::_1;


// Main subscriber class that receives drives commands and interfaces with the wheel
class DriveSubscriber : public rclcpp::Node {

    // The number of wheels on the rover
    static const int NUM_WHEELS = 6;


    //------------------------------------------------------------//
    private:

    // Stores the subscriber for the drive commands
    rclcpp::Subscription<core::msg::DriveCmd>::SharedPtr subscription_cmds;

    // Stores the subscriber to the gamepad inputs
    rclcpp::Subscription<core::msg::InputGamepad>::SharedPtr subscription_inputs;

    // Stores a counter for each step
    size_t count;

    // A flag for whether to apply the handbrake or not
    bool handbrake;

    // A flag for whether it has sent its first zero speed
    bool stopped_sent;

    // An array of wheel instances
    Wheel* wheels[NUM_WHEELS];

    
    //------------------------------------------------------------//
    private:

    /// @brief      Callback function when drive messages are received
    /// @param      msg - A pointer to the drive message
    void drive_callback (const core::msg::DriveCmd::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void input_callback (const core::msg::InputGamepad::SharedPtr msg);


    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    DriveSubscriber();
    
};