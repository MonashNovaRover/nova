#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class inherits from the base Joystick code and
    is able to send messages for the Thrustmaster
    joysticks.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
CREATION:	19/11/2021
EDITED:		19/11/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the joystick and message type
#include "joystick.h"
#include "core/msg/input_joystick.hpp"


// Thrustmaster class
class JoystickThrustmaster : public Joystick {

    //------------------------------------------------------------//
    protected:

    /// @brief      Stores the message data from the joystick
    core::msg::InputJoystick msg;
    

    //------------------------------------------------------------//
    protected:

    /// @brief      Sets the message values stored in the message object
    void set_message_values() override;


    //------------------------------------------------------------//
	public:

    /// @brief      Constructor called when the object is created
    /// @param      left - Is this the left or the right joystick
    /// @param      offset - The offset of the input axis to use
    JoystickThrustmaster(const bool left, const float offset);

    /// @brief      Gets the message object from the instance
    /// @returns    The Input Joystick message object with data
    core::msg::InputJoystick get_message();
};