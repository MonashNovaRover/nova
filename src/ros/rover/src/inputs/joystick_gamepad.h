#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class inherits from the base Joystick code and
    is able to send messages for the Xbox controller.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
CREATION:	19/11/2021
EDITED:		19/11/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the joystick and message type
#include "joystick.h"
#include "core/msg/input_gamepad.hpp"

// Gamepad class
class JoystickGamepad : public Joystick {

    //------------------------------------------------------------//
    protected:

    core::msg::InputGamepad msg;    // Stores the message data from the gamepad
    

    //------------------------------------------------------------//
    protected:

    /// @brief      Sets the message values stored in the message object
    void set_message_values() override;


    //------------------------------------------------------------//
	public:

    /// @brief      Constructor called when the object is created
    /// @param      offset - The offset of the input axis to use
    JoystickGamepad(const float offset);

    /// @brief      Gets the message object from the instance
    /// @returns    The Input Gamepad message object with data
    core::msg::InputGamepad get_message();
};
