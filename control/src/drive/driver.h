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
  - /autonomous/mode        [std_msgs/Bool] [Published]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Harrison Verrios, Josh Cherubino,
            Will de la Rue, Jory Braun
CREATION:	21/11/2021
EDITED:		13/09/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"
#include "core/msg/input_gamepad.hpp"
#include "core/msg/drive_input.hpp"
#include "std_msgs/msg/bool.hpp"

// Include CMD class
#include "cmd/cmd.h"

// The distance between the two wheel sets [m]
#define CHASSIS_SEPARATION 0.78058

// The distance between each wheel on each side [m]
#define WHEEL_SEPARATION 0.42426


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
    static const int NUM_WHEELS = 6;


    //------------------------------------------------------------//
    private:

    // Publishes whether the rover is in autonomous mode for LEDs
    rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr mode_pub;

    // A flag for whether to apply the handbrake or not
    bool handbrake;

    // A flag for whether to use autonomous state or not
    bool is_autonomous = false;

    // An array of pointers to CMD instances
    CMD* wheels[NUM_WHEELS];

    
    //------------------------------------------------------------//
    private:

    /// @brief      Sends commands to the wheels using the wheel classes
    /// @param      msg - A pointer to the drive message
    void send_commands (const core::msg::DriveInput::SharedPtr msg);

    /// @brief      Callback function when drive messages are received
    /// @param      msg - A pointer to the drive message
    void drive_callback (const core::msg::DriveInput::SharedPtr msg);

    /// @brief      Callback function when autonomous messages are received
    /// @param      msg - A pointer to the drive message
    void auto_callback (const core::msg::DriveInput::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void input_callback (const core::msg::InputGamepad::SharedPtr msg);

    /// @brief      Callback function to publish whether autonomous
    void pub_auto_mode ();

    /// @brief      Calculates the signed turning radius based on the steering factor.
    ///             The turning radius is equal to the position of the turning-circle
    ///             centre measured relative to the geometric centre of the rover wheelbase.
    ///             The radius is signed. A positive value indicates turning tp the right.
    /// @param      steer - The steer value between -1 and 1
    /// @returns    The turning radius [m]
    float get_turning_radius (float steer);

    /// @brief      Get array with velocities for each wheel, with directions and magnitude depending on the radius
    ///             Account for cases where the turning radius is beneath the rover body, or when the radius is 0
    /// @param      wheel_velocities - Array of wheel velocities, of size NUM_WHEELS. Uninitialised.
    /// @param      radius - The turning radius of the rover [m]
    /// @param      speed - Speed of each driven wheel
    /// @param      steer - Direction and amount of steering
    void fill_wheel_velocities(float wheel_velocities[NUM_WHEELS], float radius, float speed, float steer);
    
    /// @brief      Calculates the position of the wheel in relation to the wheelbase centre
    /// @param      id - The identification of the wheel
    /// @returns    The position vector (x, y)
    Vector2 get_wheel_position (int id);

    /// @brief      Calculates the distance from the wheel to the turning centre
    /// @param      pos - The position of the wheel
    /// @param      radius - The turning radius of the rover [m]
    /// @returns    The distance between wheel and the turning centre [m]
    float get_wheel_distance (Vector2 pos, float radius);
	
    /// @brief callback for when drive inputs subscription is exceeded
    void inputs_deadline_exceeded();

    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    Driver();
    
};

