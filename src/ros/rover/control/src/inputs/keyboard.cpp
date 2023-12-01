/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Matthew Gu 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "keyboard.h"
#include "print/print.h"
#include <SDL2/SDL.h>

Keyboard::Keyboard() : connected(false), reconnected(false), disconnected(true) {
    open_keyboard_device();
}

Keyboard::~Keyboard(void) {
    SDL_Quit();
}

void Keyboard::open_keyboard_device() {
    // Open the keyboard input device, requires sudo, which breaks input_publisher
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        Print::print("Keyboard Input SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    }
    window = SDL_CreateWindow("Keyboard Input Window",
                    SDL_WINDOWPOS_UNDEFINED,
                    SDL_WINDOWPOS_UNDEFINED,
                    DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT,
                    SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    if (window == NULL) {
        Print::print("Keyboard Input could not create window! SDL_Error: %s\n", SDL_GetError());
        connected = false;
    }
    else {
        connected = true;
    }
}

void Keyboard::set_message_values() {
    msg.connected = connected;

    if (msg.connected){
        msg.keys_pressed.clear();
        msg.keys_repeated.clear();
        read_key_presses();
    }
}


void Keyboard::read_key_presses()
{
    int event_count = 0;
    while (SDL_PollEvent(&event) && ++event_count < READ_CAP) {
        switch(event.type) {
            // key released, not used in this project
            case SDL_KEYUP:
                break;
            // key down
            case SDL_KEYDOWN:
                // if key press
                // See https://wiki.libsdl.org/SDL2/SDL_KeyboardEvent
                if (event.key.repeat == 0) {
                    msg.keys_pressed.push_back(key_mask(event.key.keysym));
                }
                // if key repeat
                else {
                    msg.keys_repeated.push_back(key_mask(event.key.keysym));
                }
                break;
            case SDL_QUIT:
                SDL_DestroyWindow(window);
                SDL_Quit();
                break;
            default: 
                break;
        }
    }
}

void Keyboard::update() {
    // Set the message values from the message objects
    set_message_values();

    if (!connected) {
        open_keyboard_device();
    }

    // Get connected state
    bool new_connected = (window != NULL);

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

int Keyboard::key_mask(SDL_Keysym key) {
    // virtual key code
    int key_code = key.sym;
    int mod = key.mod;
    if (mod & KMOD_CTRL){
        key_code |= CTRL_MASK;
    }
    if (mod & KMOD_SHIFT){
        key_code |= SHIFT_MASK;
    }
    if (mod & KMOD_ALT){
        key_code |= ALT_MASK;
    }
    return key_code;
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
