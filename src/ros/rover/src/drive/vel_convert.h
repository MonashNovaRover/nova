#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: 
TOPICS:
  - 
  - 
SERVICES:
ACTIONS:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Himsara Gallege
CREATION:	08/12/2021
EDITED:		08/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"
//#include "core/msg/input_gamepad.hpp"
#include "core/msg/drive_cmd.hpp"

#include <iostream>

// Use the standard namespaces
using namespace std;
using namespace std::chrono_literals;
using std::placeholders::_1;

// The minimum and maximum multipliers
const float MIN_MULTIPLIER       = 0.1;  // The minimum multiplier value
const float conversion_intercept = 0.0305
const float CONVERSION_FACTOR    = 1.3759
const float STEER_FACTOR         = 0.5




// Main publisher class that sends input data for the gamepad and joysticks
class VelConvert : public rclcpp::Node {

    //------------------------------------------------------------//
    private:

    // Stores the loop timer for the update function
    rclcpp::TimerBase::SharedPtr timer;

    // Stores the publisher for the drive commands
    rclcpp::Publisher<core::msg::DriveCmd>::SharedPtr publisher;

    // Stores the subscriber to the gamepad inputs
    rclcpp::Subscription<core::msg::DriveVel>::SharedPtr subscription;

    // Stores a counter for each step
    size_t count;
    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    DrivePublisher();
    
};