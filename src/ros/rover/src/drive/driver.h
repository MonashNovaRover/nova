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
            Will de la Rue
CREATION:	21/11/2021
EDITED:		09/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"
#include "core/msg/input_gamepad.hpp"
#include "core/msg/drive_input.hpp"
#include "std_msgs/msg/bool.hpp"

// Include wheel class
#include "wheel.h"

// The distance between the two wheel sets [m]
#define CHASSIS_SEPARATION 0.78058

// The distance between each wheel on each side [m]
#define WHEEL_SEPARATION 0.42426

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
    static const int NUM_WHEELS = 6;

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

    // Publishes whether the rover is in autonomous mode for LEDs
    rclcpp::TimerBase::SharedPtr mode_timer;
    rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr mode_pub;

    // The period at which we publish whether  we are in autonomous mode
    std::chrono::milliseconds mode_timer_period = 200ms;
    // A flag for whether to apply the handbrake or not
    bool handbrake;

    // A flag for whether it has sent its first zero speed
    bool stopped_sent;

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

    /// @brief      Callback function when autonomous messages are received
    /// @param      msg - A pointer to the drive message
    void auto_callback (const core::msg::DriveInput::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void input_callback (const core::msg::InputGamepad::SharedPtr msg);

    /// @brief      Callback function to publish whether autonomous
    void pub_auto_mode ();

    /// @brief      Calculates the center turning circle distance based
    ///             on the steering factor. This is from the center of mass.
    /// @param      steer - The steer value between -1 and 1
    /// @returns    The distance between center of mass and circle [m]
    float get_locas_distance (float steer);

    /// @brief      Calculates the position of the wheel in relation to the CoM
    /// @param      id - The identification of the wheel
    /// @returns    The position vector (x, y)
    Vector2 get_wheel_position (int id);

    /// @brief      Calculates the distance from the wheel to the locas
    /// @param      pos - The position of the wheel
    /// @param      locas - The distance from CoM to locas [m]
    /// @returns    The distance between wheel and the locas [m]
    float get_wheel_distance (Vector2 pos, float locas);

    /// @brief      Calculates the tangent scale of the wheel turning
    /// @param      pos - The position of the wheel
    /// @param      locas - The distance from CoM to locas [m]
    /// @returns    The tangent scale
    float get_tangent_scale (Vector2 pos, float locas);
	
    /// @brief callback for when drive inputs subscription is exceeded
    void inputs_deadline_exceeded();

    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    Driver();
    
};

