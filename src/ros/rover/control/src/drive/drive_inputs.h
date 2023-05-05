#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class reads data from the inputs from the game
    controllers and is able to publish drive commands
    based on what the controller data tells us.
The drive commands will be a RPM (desired) and a steer
    factor. Each of these will be a value between 0
    and 1.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: drive_pub
TOPICS:
  - /control/input_gamepad  [InputGamepad]  [Subscribed]
  - /control/drive_cmds     [DriveInput]    [Published]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Harrison Verrios, Liam Whittle
CREATION:	14/11/2021
EDITED:		31/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"
#include "core/msg/input_gamepad.hpp"
#include "core/msg/drive_input.hpp"


// The minimum and maximum multipliers
const float MIN_MULTIPLIER      = 0.1;  // The minimum multiplier value
const float MAX_MULTIPLIER      = 1.0;  // The maximum multiplier value
const float DELTA_MULTIPLIER    = 0.1;  // The change in multiplier

// The initial multipliers
const float INITIAL_MULT_SPEED = 0.3;

// The minimum trigger speed multiplier to apply when the right trigger is held
const float MIN_TRIGGER_MULTIPLIER = 0.4;

// The enum denotes the current drive mode of the rover

/*
    HOW TO DRIVE THE ROVER:

    Left Y Axis:    Speed (forwards and backwards)
    Right X Axis:   Steer (left and right)
    DPAD Y Axis:    Increase / Decrease the Speed multiplier by 10%
    DPAD X Axis:    Increase / Decrease the Steer multiplier by 10%
    Right Trigger:  Add Custom speed multipliers between 1.0 and 0.4

    Back:           Lock the controller
    Start:          Unlock the controller
*/

// Main publisher class that sends input data for the gamepad and joysticks
class DriveInputs : public rclcpp::Node {

    //------------------------------------------------------------//
private:

    // Stores the loop timer for the update function
    rclcpp::TimerBase::SharedPtr timer;

    // Stores the publisher for the drive commands
    rclcpp::Publisher<core::msg::DriveInput>::SharedPtr publisher;

    // Stores the subscriber to the gamepad inputs
    rclcpp::Subscription<core::msg::InputGamepad>::SharedPtr gamepad_input_subscription;
    rclcpp::Subscription<core::msg::DriveInput>::SharedPtr autonomous_commands_subscription;

    // A flag for whether the controller is connected
    bool connected = false;

    // A flag which indicates if a zero message has been received in the previous frame
    bool prev_msg_received = false;

    // A lock on the controls - can be unlocked
    bool locked = true;

    // A flag for the handbrake
    bool handbrake = false;

    // Autonomous mode
    bool autonomous = false;

    float radius = INFINITY;

    int direction = 0;

    float speed = 0.0;

    // Drive mode
    unsigned char mode = core::msg::DriveInput::TANK;

    // Stores the current state of the trigger multiplier
    float trigger_speed = 1.0;

    // The current speed and steer multipliers
    float multiplier_speed = INITIAL_MULT_SPEED;
    //------------------------------------------------------------//
private:
    /// @brief      Adjusts one of the multipliers between 0.1 and 1.0
    /// @param      multiplier - A reference to the speed or steer multiplier
    /// @param      increase - A boolean flag for increasing (or false to decrease)
    /// @returns    The current value of the multiplier
    float adjust_multiplier (float& multiplier, bool increase);

    /// @brief      Publishes the drive commands after analysing
    ///                 the input data.
    void publish_cmds ();

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void input_callback (const core::msg::InputGamepad::SharedPtr msg);

    /// @brief      Callback function when autonomous messages are received.
    /// @param      msg - A pointer to the autonomous message
    void autonomous_callback (const core::msg::DriveInput::SharedPtr msg);

    /// @brief      Callback function when deadline for input subscription is exceeded
    void input_deadline_exceeded();

    /// @brief      Callback function when deadline for auto subscription is exceeded
    void auto_deadline_exceeded();
    //------------------------------------------------------------//
public:

    /// @brief      Default constructor function that starts up the node
    DriveInputs();
    
};
