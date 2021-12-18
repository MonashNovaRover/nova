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
#include "core/msg/drive_input.hpp"
#include "core/msg/drive_vel.hpp"

#include <iostream>

// Use the standard namespaces
using namespace std;
using namespace std::chrono_literals;
using std::placeholders::_1;


// Main publisher class that sends input data for the gamepad and joysticks
class DriveVelocity : public rclcpp::Node {

    //------------------------------------------------------------//
    private:

    // The minimum and maximum multipliers
    const float MIN_MULTIPLIER       = 0.1;  // The minimum multiplier value
    const float CONVERSION_INTERCEPT = 0.0305;
    const float CONVERSION_FACTOR    = 1.3759;
    const float STEER_FACTOR         = 0.5;


    // Stores the publisher for the drive commands
    rclcpp::Publisher<core::msg::DriveInput>::SharedPtr publisher;

    rclcpp::Subscription<core::msg::DriveVel>::SharedPtr subscription;

    // Stores a counter for each step
    size_t count;
    //------------------------------------------------------------//
    public:
    
    void velocity_callback (const core::msg::DriveVel::SharedPtr msg);
    
    /// @brief      Default constructor function that starts up the node
    DriveVelocity();
    
};