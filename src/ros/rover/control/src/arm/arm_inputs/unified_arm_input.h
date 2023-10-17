#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class decides which input device to use

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Matthew Gu
CREATION:	23/09/2023
EDITED:		30/09/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
High: Swap JoyStick message to use Joy message type
High: Make joystick override changeable in launch file/runtime
High: See if it is more appropriate to move the subscribes to the translator
Medium: Make key mapping changeable in runtime (json config file)?
Low: Possibly make another class as parent class of 
each individual input type, and then have array for priority. 


Note: implement a get_device to be called
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "input_device.h"
#include "core/msg/input_joystick.hpp"
#include "core/msg/input_keyboard.hpp"

// Define an enum for the input device indices
enum InputDeviceIndex {
    JOYSTICK_INDEX = 0,
    KEYBOARD_INDEX = 1
};

class UnifiedArmInput {
    //------------------------------------------------------------//
    private:

    bool joystick_override;

    // Store the input devices
    InputDevice input_devices [2];

    // Store state of last-received messages from the arm inputs
    core::msg::InputJoystick joystick_l;
    core::msg::InputJoystick joystick_r;
    core::msg::InputKeyboard keyboard;

    //------------------------------------------------------------//
    public:

    /// @brief     Constructor for the ArmInputs class.
    UnifiedArmInput(bool joystick_override);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_l_callback (const core::msg::InputJoystick::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_r_callback (const core::msg::InputJoystick::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void keyboard_callback (const core::msg::InputKeyboard::SharedPtr msg);

    void deadline_callback (int device_index);

    /// @brief      Returns the input device to use
    InputDevice& get_input_device ();
}
