#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

TODO: Add description, and refactor to InputDevices
interface 

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Matthew Gu
CREATION:	23/09/2023
EDITED:		30/09/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include <libudev.h>
#include <stdio.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>

#include "core/msg/input_keyboard.hpp"

// Use the standard namespace
using namespace std;

class Keyboard {
    //------------------------------------------------------------//
    protected:

    /// @brief      Stores the message data from the joystick
    core::msg::InputKeyboard msg;

    /// @brief      A flag for whether the keyboard is connected
    bool connected;
    /// @brief      A flag for whether this frame the keyboard connected                   
    bool reconnected;
    /// @brief      A flag for whether this frame the keyboard disconnected
    bool disconnected;                      

    /// @brief    The file descriptor for the keyboard input device
    int fd;

    /// @brief    The linux input event struct
    struct input_event ev;

    //------------------------------------------------------------//
    protected:

    /// @brief      Sets the message values stored in the message object
    void set_message_values();

    /// @brief     Returns whether the key is down
    /// @param key_code the linux event code for key, can be found in linux/input.h
    bool is_key_down(int key_code);

    /// @brief    Resets the key states
    void reset_key_states();

    //------------------------------------------------------------//
	public:

    /// @brief      Constructor that finds and intialzes the device
    Keyboard();

    /// @brief      Updates the input data and stores data to the message object
    void update();

    /// @brief      Returns whether the joystick is connected
    /// @returns    Connected flag
    bool is_connected ();

    /// @brief      Returns whether the joystick has been reconnected in this frame
    /// @returns    Reconnection flag
    bool is_reconnected ();

    /// @brief      Returns whether the joystick has been disconnected in this frame
    /// @returns    Disconnection flag
    bool is_disconnected ();

    /// @brief      Gets the message object from the instance
    /// @returns    The Input Joystick message object with data
    core::msg::InputKeyboard get_message();
};