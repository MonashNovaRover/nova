#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This code uses the Gamepad library to detect inputs
    from various joysticks plugged into the computer.
It is able to store information related to each of
    the controllers, including button states, axis
    values and triggers.
This code requires the message types from the core
    repository.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Marcel Masque, Harrison Verrios
CREATION:	29/01/2020
EDITED:		14/11/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Item One
 - Item Two
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include Message Types
#include "core/msg/input_gamepad.hpp"

// General includes
#include <gamepad/gamepad.h>
#include <cmath>

// Use the standard namespace
using namespace std;

// Main joystick class
class Joystick {

    // Stores the maximum axis stick values, based on the gamepad deadzones
    const float STICK_MAX_L = 32767 - GAMEPAD_DEADZONE_LEFT_STICK;
    const float STICK_MAX_R = 32767 - GAMEPAD_DEADZONE_RIGHT_STICK;

    //------------------------------------------------------------//
    protected:

        core::msg::InputGamepad msg;            // Stores the message data from the gamepad
        GAMEPAD_DEVICE controller;              // Stores the device controller ID

        float offset;                           // Stores the offset of the axis
        int stick_lx;                           // Stores the raw input of the left stick - x axis
        int stick_ly;                           // Stores the raw input of the left stick - y axis
        int stick_rx;                           // Stores the raw input of the right stick - x axis
        int stick_ry;                           // Stores the raw input of the right stick - y axis

        float stick_lx_f;                       // Stores the value of the left stick - x axis
		float stick_ly_f;                       // Stores the value of the left stick - y axis
		float stick_rx_f;                       // Stores the value of the right stick - x axis
		float stick_ry_f;                       // Stores the value of the right stick - y axis
    
        bool twist_lock;                        // Whether the twist on the axis has been locked
	    bool hat_lock;                          // Whether the hat cap has been locked
    

    //------------------------------------------------------------//
    protected:

        /// @brief      Corrects the data for any deadzone of the axis
        void CorrectDeadzone();

        /// @brief      Sets the message values stored in the message object
        void SetMessageValues();

        /// @brief      Gets the state of the button as an interger
        ///                 0 - Not Pressed
        ///                 1 - Button Triggered
        ///                 2 - Button Down
        ///                 3 - Button Released
        /// @param      button - The button type looking for
        /// @returns    The button state
        int GetButtonState (const GAMEPAD_BUTTON button);

        /// @brief      Returns the sign of an input float (-1 or 1)
        /// @param      val - The value to be calculated
        /// @returns    The integer sign value
		int Sign(const float val);


    //------------------------------------------------------------//
	public:

        /// @brief      Constructor that takes in a controller device
        /// @param      controller - The controller of this joystick
		Joystick(const GAMEPAD_DEVICE controller);

        /// @brief      Constructor that takes in multiple inputs
        /// @param      controller - The controller of this joystick
        /// @param      offset - The offset of the input axis to use
		Joystick(const GAMEPAD_DEVICE controller, const float offset);

        /// @brief      Updates the input data and stores data to the message object
		void Update();

        /// @brief      Gets the message object from the instance
        /// @returns    The Input Gamepad message object with data
		core::msg::InputGamepad GetMessage();

};