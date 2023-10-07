/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Matthew Gu 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO: 
Should I use an array or just individual keys for key states?
Add capability to detect usb keyboards and select that as the input device (not doable yet due to being on VM)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "keyboard.h"
#include <iostream>

Keyboard::Keyboard(const char* devPath) : connected(false), reconnected(false), disconnected(false) {
    devicePath = devPath;
    // Open the keyboard input device, requires sudo, which breaks input_publisher
    open_keyboard_device(devicePath);
}

void Keyboard::open_keyboard_device(const char* devPath){
    fd = open(devPath, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        cerr << "Error opening keyboard input device" << strerror(errno) << endl;
        connected = false;
        return;
    } else {
        connected = true;
    }
}

void Keyboard::set_message_values() {
    msg.connected = connected;

    if (msg.connected){
        // note that key codes are not in order in linux input, see linux/input_event_codes.h
        for (int key_code = KEY_ESC; key_code <= KEY_CAPSLOCK; key_code++) {
            msg.key_number_state[key_code-KEY_ESC+1] = is_key_down(key_code);
        }
    }
}

void Keyboard::update() {
    // Set the message values from the message objects
    set_message_values();

    if (!connected) {
        open_keyboard_device(devicePath);
    }

    // Get connected state
    bool new_connected = (fd != -1);

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
    struct input_event ev;
    ssize_t bytesRead = read(fd, &ev, sizeof(ev));
    // if there is no input, return false
    if (bytesRead == -1) {
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
