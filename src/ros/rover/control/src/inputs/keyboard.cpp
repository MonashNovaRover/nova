/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Matthew Gu 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO: 
Should I use a vector or just individual keys for key states?
Add capability to detect usb keyboards and select that as the input device
Switch to arrays for the keys
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "keyboard.h"
#include <iostream>

// fix the checking for connection
void Keyboard::Keyboard() : connected(true), reconnected(false), disconnected(false) {
    // Open the keyboard input device
    fd = open(INPUT_DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        cerr << "Error opening keyboard input device" << endl;
        return;
    }
}

void Keyboard::set_message_values() {
    // Clear the key states
    reset_key_states();

    if (msg.connected){
        key_ctrl_state = is_key_down(KEY_LEFTCTRL) || is_key_down(KEY_RIGHTCTRL);
        key_alt_state = is_key_down(KEY_LEFTALT) || is_key_down(KEY_RIGHTALT);
        key_tab_state = is_key_down(KEY_TAB);

        key_number_state.clear();
        // note that key codes are not in order in linux input
        key_number_state.push_back(is_key_down(KEY_0));
        for (int key_code = KEY_1; key_code <= KEY_9; key_code++) {
            key_number_state.push_back(is_key_down(key_code));
        }

        key_alphabet_state.clear();
        // note that key codes are not in alphabetical order in linux input (in fact it is in QWERTY order)
        key_alphabet_state.push_back(is_key_down(KEY_A));
        key_alphabet_state.push_back(is_key_down(KEY_B));
        key_alphabet_state.push_back(is_key_down(KEY_C));
        key_alphabet_state.push_back(is_key_down(KEY_D));
        key_alphabet_state.push_back(is_key_down(KEY_E));
        key_alphabet_state.push_back(is_key_down(KEY_F));
        key_alphabet_state.push_back(is_key_down(KEY_G));
        key_alphabet_state.push_back(is_key_down(KEY_H));
        key_alphabet_state.push_back(is_key_down(KEY_I));
        key_alphabet_state.push_back(is_key_down(KEY_J));
        key_alphabet_state.push_back(is_key_down(KEY_K));
        key_alphabet_state.push_back(is_key_down(KEY_L));
        key_alphabet_state.push_back(is_key_down(KEY_M));
        key_alphabet_state.push_back(is_key_down(KEY_N));
        key_alphabet_state.push_back(is_key_down(KEY_O));
        key_alphabet_state.push_back(is_key_down(KEY_P));
        key_alphabet_state.push_back(is_key_down(KEY_Q));
        key_alphabet_state.push_back(is_key_down(KEY_R));
        key_alphabet_state.push_back(is_key_down(KEY_S));
        key_alphabet_state.push_back(is_key_down(KEY_T));
        key_alphabet_state.push_back(is_key_down(KEY_U));
        key_alphabet_state.push_back(is_key_down(KEY_V));
        key_alphabet_state.push_back(is_key_down(KEY_W));
        key_alphabet_state.push_back(is_key_down(KEY_X));
        key_alphabet_state.push_back(is_key_down(KEY_Y));
        key_alphabet_state.push_back(is_key_down(KEY_Z));
    }
    msg.key_alphabet_state = key_alphabet_state;
    msg.key_number_state = key_number_state;
    msg.key_ctrl_state = key_ctrl_state;
    msg.key_alt_state = key_alt_state;
    msg.key_tab_state = key_tab_state;
}

void Keyboard::update() {
    // Set the message values from the message objects
    set_message_values();

    // Get connected state
    bool new_connected = is_connected();

    // Look for reconnection
    if (!connected && new_connected)
        reconnected = true;
    else
        reconnected = false;

    // Look for disconnection
    if (connected && !new_connected)
        disconnected = true;
    else
        disconnected = false;

    // Updated the connection state
    connected = new_connected;
}

// Get the state of a key
bool Keyboard::is_key_down(int key_code) {
    // read the current input
    ssize_t bytesRead = read(fd, &ev, sizeof(ev));
    if (bytesRead == -1) {
        std::cerr << "Error reading input event." << std::endl;
        close(fd);
        return false;
    }

    // Check for key presses or releases
    if (ev.type == EV_KEY && ev.code == key_code) {
        if (ev.value == 1) {
            return true;
        } else if (ev.value == 0) {
            return false;
        }
    }
}

void Keyboard::reset_key_states() {
    // Clear the key states
    key_alphabet_state.clear();
    key_number_state.clear();
    key_alt_state = false;
    key_ctrl_state = false;
    key_tab_state = false;
}

// return the message object
core::msg::InputKeyboard Keyboard::get_message() {
    return msg;
}

// Return connection flag
bool Keyboard::is_connected () {
    return connected;
}

// Return reconnection flag
bool Keyboard::is_reconnected () {
    return reconnected;
}

// Return disconnection flag
bool Keyboard::is_disconnected () {
    return disconnected;
}
