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
        // most likely permission denied, read the errono and chmod 777 the device input file. 
        cerr << "Error opening keyboard input device. " << strerror(errno) << endl;
        connected = false;
        return;
    } else {
        connected = true;
        return;
    }
}

void Keyboard::set_message_values() {
    msg.connected = connected;

    if (msg.connected){
        msg.keys_pressed.clear();
        msg.keys_repeated.clear();
        msg.keys_released.clear();
        read_key_presses();
    }
}

void Keyboard::read_key_presses()
{
    // read the current input
    struct input_event ev;
    int num_read = 0;
    while (read(fd, &ev, sizeof(ev)) != -1 && num_read < READ_CAP){
        // Check for key presses
        if (ev.type == EV_KEY) {
            switch (ev.value)
            {
            case 0:
                msg.keys_released.push_back(ev.code);
                break;
            case 1:
                msg.keys_pressed.push_back(ev.code);
                break;
            case 2:
                msg.keys_repeated.push_back(ev.code);
                break;
            default:
                break;
            }
        }
        num_read++;
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
