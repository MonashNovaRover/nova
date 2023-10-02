/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Matthew Gu 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO: 
Should I use a vector or just individual keys for key states?
Add capability to detect usb keyboards and select that as the input device
Switch to uint32_t for the keys (by bit shifting) make be hard to debug though
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "keyboard.h"
#include <iostream>

// fix the checking for connection
Keyboard::Keyboard() : connected(true), reconnected(false), disconnected(false) {
    // Open the keyboard input device
    fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        cerr << "Error opening keyboard input device" << endl;
        return;
    }
}

// key board has not need for additional operations on the key values (as it will be 0 or 1)
void Keyboard::set_message_values() {
    // Clear the key states
    reset_key_states();
    msg.connected = connected;

    if (msg.connected){
        msg.key_ctrl_state = is_key_down(KEY_LEFTCTRL) || is_key_down(KEY_RIGHTCTRL);
        msg.key_alt_state = is_key_down(KEY_LEFTALT) || is_key_down(KEY_RIGHTALT);
        msg.key_tab_state = is_key_down(KEY_TAB);

        // note that key codes are not in order in linux input
        msg.key_number_state[0] = is_key_down(KEY_0);
        for (int key_code = KEY_1; key_code <= KEY_9; key_code++) {
            msg.key_number_state[key_code-KEY_1+1] = is_key_down(key_code);
        }

        // note that key codes are not in alphabetical order in linux input (in fact it is in QWERTY order)
        msg.key_alphabet_state[0] = is_key_down(KEY_A);
        msg.key_alphabet_state[1] = is_key_down(KEY_B);
        msg.key_alphabet_state[2] = is_key_down(KEY_C);
        msg.key_alphabet_state[3] = is_key_down(KEY_D);
        msg.key_alphabet_state[4] = is_key_down(KEY_E);
        msg.key_alphabet_state[5] = is_key_down(KEY_F);
        msg.key_alphabet_state[6] = is_key_down(KEY_G);
        msg.key_alphabet_state[7] = is_key_down(KEY_H);
        msg.key_alphabet_state[8] = is_key_down(KEY_I);
        msg.key_alphabet_state[9] = is_key_down(KEY_J);
        msg.key_alphabet_state[10] = is_key_down(KEY_K);
        msg.key_alphabet_state[11] = is_key_down(KEY_L);
        msg.key_alphabet_state[12] = is_key_down(KEY_M);
        msg.key_alphabet_state[13] = is_key_down(KEY_N);
        msg.key_alphabet_state[14] = is_key_down(KEY_O);
        msg.key_alphabet_state[15] = is_key_down(KEY_P);
        msg.key_alphabet_state[16] = is_key_down(KEY_Q);
        msg.key_alphabet_state[17] = is_key_down(KEY_R);
        msg.key_alphabet_state[18] = is_key_down(KEY_S);
        msg.key_alphabet_state[19] = is_key_down(KEY_T);
        msg.key_alphabet_state[20] = is_key_down(KEY_U);
        msg.key_alphabet_state[21] = is_key_down(KEY_V);
        msg.key_alphabet_state[22] = is_key_down(KEY_W);
        msg.key_alphabet_state[23] = is_key_down(KEY_X);
        msg.key_alphabet_state[24] = is_key_down(KEY_Y);
        msg.key_alphabet_state[25] = is_key_down(KEY_Z);
    }
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
uint8_t Keyboard::is_key_down(int key_code) {
    // read the current input
    ssize_t bytesRead = read(fd, &ev, sizeof(ev));
    if (bytesRead < 0) {
        int err = errno;
        fprintf(stderr, "%s\n", explain_errno_read(err, fd, ev, sizeof(ev)));
        return false;
    }

    // Check for key presses
    if (ev.type == EV_KEY && ev.code == key_code) {
        // key press or key repeat
        if (ev.value == 1 || ev.value == 2) {
            return true;
        }
    }
    // key release or no event
    return false;
}

void Keyboard::reset_key_states() {
    // Clear the key states
    msg.key_alphabet_state.clear();
    msg.key_number_state.clear();
    msg.key_alt_state = false;
    msg.key_ctrl_state = false;
    msg.key_tab_state = false;
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
