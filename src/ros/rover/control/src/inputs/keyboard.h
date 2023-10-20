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


#include "core/msg/input_keyboard.hpp"

// Use the standard namespace
using namespace std;

class Keyboard {
    //------------------------------------------------------------//
    private:
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

    /// @brief Cap the number of keys to read to avoid infinite loop (last 8 key presses)
    const static int READ_CAP = 8;

    //------------------------------------------------------------//
    protected:

    /// @brief      Opens the keyboard input device
    /// @param      devPath the path to the keyboard input device
    void open_keyboard_device(const char* devPath);

    /// @brief      Sets the message values stored in the message object
    void set_message_values();

    /// @brief     Grabs all key presses generated since last sampling
    void read_key_presses();

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
