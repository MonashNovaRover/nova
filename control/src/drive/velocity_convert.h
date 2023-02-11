#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class converts a velocity message in RPM to
    the correct drive commands, based on the current
    wheel dimensions.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: velocity_convert
TOPICS:
  - /autonomous/drive_velocity      [DriveVel]      [Subscribed]
  - /autonomous/drive_inputs        [DriveInput]    [Published]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Himsara Gallege, Harrison Verrios
CREATION:	08/12/2021
EDITED:		05/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Test on the rover
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"
#include "core/msg/drive_input.hpp"
#include "core/msg/drive_vel.hpp"


// Main publisher class that sends input data for the gamepad and joysticks
class VelocityConvert : public rclcpp::Node {

    //------------------------------------------------------------//
    private:

    // The maximum speed that the rover drives at [m/s]
    const float MAX_SPEED               = 1.3759;

    // The radius of the wheel [m]
    const float WHEEL_RADIUS            = 0.122;

    // The factor to convert angular velocity to spin speed
    const float STEER_FACTOR            = 0.25;
    

    // Stores the publisher for the drive commands
    rclcpp::Publisher<core::msg::DriveInput>::SharedPtr publisher;


    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    VelocityConvert();
    
    /// @brief      Callback function for when a velocity message is read
    /// @param      msg - A pointer to the velocity message
    void velocity_callback (const core::msg::DriveVel::SharedPtr msg);
    
};