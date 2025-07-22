#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This code uses SDL2 to detect inputs from the keyboard
See https://wiki.libsdl.org/SDL_KeyboardEvent for more info
This code requires the message types from the core
    repository.
Previous version uses Linux Input for synchronous sampling, 
    and was replaced as that required sudo permission. 
Requires InputKeyboard message type from core. 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	inputs
AUTHOR(S):	Matthew Gu
CREATION:	23/09/2023
EDITED:		02/12/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include <SDL2/SDL.h>
#include "input_interfaces/msg/input_keyboard.hpp"

class Keyboard {
    //------------------------------------------------------------//
    private:
    /// @brief      Stores the message data from the joystick
    input_interfaces::msg::InputKeyboard msg;

    /// @brief      A flag for whether the keyboard is connected
    bool connected;
    /// @brief      A flag for whether this frame the keyboard connected                   
    bool reconnected;
    /// @brief      A flag for whether this frame the keyboard disconnected
    bool disconnected;                      

    /// @brief    The SDL window instance created for reading
    SDL_Window* window;

    /// @brief   The SDL event instance created for reading
    SDL_Event event;

    /// @brief  A snapshot of the on the keyboard
    const uint8_t* state;

    /// @brief  The number of key events read
    int num_keys;

    /// @brief  The SDL initial window width
    const static int DEFAULT_WINDOW_WIDTH = 640;

    /// @brief The SDL initial window height
    const static int DEFAULT_WINDOW_HEIGHT = 480;

    /// @brief      Opens the keyboard input device
    void open_keyboard_device();

    /// @brief      Sets the message values stored in the message object
    void set_message_values();

    /// @brief     Grabs all key presses generated since last sampling
    void read_key_presses();

    /// @brief     Returns the key after anded with masks
    ///            Allows Key to be separated from Ctrl+Key
    /// @param     key The key to mask
    /// @param     mod The modifier to mask
    /// @returns   The masked key
    uint32_t key_mask(uint8_t key, uint16_t mod);

    //------------------------------------------------------------//
	public:

    /// @brief The key mask for the control key
    const static int CTRL_MASK = 1<<31;

    /// @brief The key mask for the shift key
    const static int SHIFT_MASK = 1<<30;

    /// @brief The key mask for the alt key
    const static int ALT_MASK = 1<<29;

    /// @brief      Constructor that finds and intialzes the device
    Keyboard();

    /// @brief     Destructor that closes the device
    ~Keyboard();

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
    input_interfaces::msg::InputKeyboard get_message();
};
