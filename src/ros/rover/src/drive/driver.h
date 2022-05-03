#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class interfaces with the wheels and ROS and
    is able to publish data over CAN to the wheels.
It can also read autonomous messages if the velocity
    convert script is running.
The wheels are indexed with 0, 1, 2 on the left and
    3, 4, 5 on the right, with the largest at the back.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: drive_sub
TOPICS:
  - /control/input_gamepad  [InputGamepad]  [Subscribed]
  - /control/drive_cmds     [DriveCmd]      [Subscribed]
  - /autonomous/drive_cmds  [DriveCmd]      [Subscribed]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Harrison Verrios, Josh Cherubino,
            Will de la Rue, Liam Whittle
CREATION:	21/11/2021
EDITED:		09/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// include standard libraries
#include <array> 

// Include ROS packages
#include "rclcpp/rclcpp.hpp"
#include "core/msg/input_gamepad.hpp"
#include "core/msg/drive_input.hpp"

// Include wheel class
#include "wheel.h"

// The distance between the two wheel sets [m]
#define CHASSIS_SEPARATION 0.78058

// The distance between each wheel on each side [m]
#define WHEEL_SEPARATION 0.42426

#define NUM_WHEELS_DEF 6 


// Use the standard namespaces
using namespace std;
using namespace std::chrono_literals;
using std::placeholders::_1;


// Store a position structure with x and y
struct Vector2 {

    //------------------------------------------------------------//
    public:

    // The position values
    float x, y;

    // Constructor for the vector
    Vector2 (float x, float y) {
        this->x = x;
        this->y = y;
    }
};


// Main subscriber class that receives drives commands and interfaces with the wheel
class Driver : public rclcpp::Node {

    // The number of wheels on the rover
    static const int NUM_WHEELS = NUM_WHEELS_DEF;

    // Whether to use the tangent scaling
    bool USE_TANGENT_SCALING = false;


    //------------------------------------------------------------//
    private:

    // Stores the subscriber for the drive commands (manual)
    rclcpp::Subscription<core::msg::DriveInput>::SharedPtr subscription_cmds_man;

    // Stores the subscriber for the drive commands (auto)
    rclcpp::Subscription<core::msg::DriveInput>::SharedPtr subscription_cmds_auto;

    // Stores the subscriber to the gamepad inputs
    rclcpp::Subscription<core::msg::InputGamepad>::SharedPtr subscription_inputs;

    // A flag for whether to apply the handbrake or not
    bool handbrake;

    // A flag for whether it has sent its first zero speed
    bool stop_sent;

    // A flag for whether to use autonomous state or not
    bool is_autonomous = false;

    // An array of wheel instances
    Wheel* wheels[NUM_WHEELS];
	
	
    
    //------------------------------------------------------------//
    private:

    /// @brief      Sends commands to the wheels using the wheel classes
    /// @param      msg - A pointer to the drive message
    void send_commands (const core::msg::DriveInput::SharedPtr msg);

    /// @brief      Callback function when drive messages are received
    /// @param      msg - A pointer to the drive message
    void drive_callback (const core::msg::DriveInput::SharedPtr msg);
    
    /// @brief      Pure function which calculates the total velocities the wheels should drive at
    /// @param      msg - A pointer to the drive message
    std::array<float, NUM_WHEELS_DEF> calculate_velocities(const core::msg::DriveInput::SharedPtr msg) const;

    /// @brief      Callback function when autonomous messages are received
    /// @param      msg - A pointer to the drive message
    void auto_callback (const core::msg::DriveInput::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void input_callback (const core::msg::InputGamepad::SharedPtr msg);

    /// @brief      Calculates the center turning circle distance based
    ///             on the steering factor. This is from the center of mass.
    /// @param      steer - The steer value between -1 and 1
    /// @returns    The distance between center of mass and circle [m]
    float get_wheel_centre_distance(float steer) const;

    /// @brief      Calculates the position of the wheel in relation to the CoM
    /// @param      id - The identification of the wheel
    /// @returns    The position vector (x, y)
    Vector2 get_wheel_position (int id) const;

    /// @brief      Calculates the distance from the wheel to the wheel_centre (center of the circle) 
    /// @param      pos - The position of the wheel
    /// @param      wheel_centre - The distance from CoM to wheel_centre [m]
    /// @returns    The distance between wheel and the wheel_centre [m]
    float get_wheel_distance (Vector2 pos, float wheel_centre) const;

    /// @brief      Calculates the tangent scale of the wheel turning
    /// @param      pos - The position of the wheel
    /// @param      wheel_centre - The distance from CoM to wheel_centre [m]
    /// @returns    The tangent scale
    float get_tangent_scale (Vector2 pos, float wheel_centre) const;

    /// @brief      drives each wheel at the provided velocity
    /// @param      velocities - an array of length NUM_WHEELS containing a velocity to drive each wheel
    void drive_wheels(std::array<float, NUM_WHEELS_DEF> velocities);
    
    /// @brief      sends an all stop command to each wheel
    void all_stop();

    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    Driver();
    
};
