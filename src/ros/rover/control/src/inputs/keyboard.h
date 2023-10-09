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

#include <stdio.h>
#include <linux/input.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <unistd.h>

#include "core/msg/input_keyboard.hpp"

// Use the standard namespace
using namespace std;

class Keyboard {
    //------------------------------------------------------------//
    protected:

    /// @brief      Stores the message data from the joystick
    core::msg::InputKeyboard msg;

    const char* devicePath;

    /// @brief      A flag for whether the keyboard is connected
    bool connected;
    /// @brief      A flag for whether this frame the keyboard connected                   
    bool reconnected;
    /// @brief      A flag for whether this frame the keyboard disconnected
    bool disconnected;                      

    /// @brief    The file descriptor for the keyboard input device
    int fd;

    //------------------------------------------------------------//
    protected:

    /// @brief      Opens the keyboard input device
    /// @param      devPath the path to the keyboard input device
    void open_keyboard_device(const char* devPath);

    /// @brief      Sets the message values stored in the message object
    void set_message_values();

    /// @brief     Returns whether the key is down
    /// @param key_code the linux event code for key, can be found in linux/input.h
    int get_key_state(int key_code);

    //------------------------------------------------------------//
	public:

    /// @brief      Constructor that finds and intialzes the device
    Keyboard(const char* devPath);

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