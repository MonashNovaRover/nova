/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This is the description of the class. Please explain the purpose
  of the class, what it will be used for, and how it can
  be interfaced with.
Try to keep this description to a maximum of 5 lines long. Use
  multiple lines if needed like this example.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: node_name
TOPICS:
  - /topic_name [Message Type]
SERVICES:
  - /service_name [Service Type]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
CREATION:	13/11/2021
EDITED:		13/11/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Test with Joysticks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include all relevant packages; these are all standard C++ packages that come with Linux
#include <iostream>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/joystick.h>

// Use the appropriate namespace
using namespace std;

// Define a list of Joystick types
enum JoystickType {
    CONTROLLER,
    JOYSTICK_LEFT,
    JOYSTICK_RIGHT
};

// Define a list of Joystick inputs
enum JoystickInput {
    C_BTN_A,
    C_BTN_B,
    C_BTN_X,
    C_BTN_Y,
    C_BTN_SHOULDER_L,
    C_BTN_SHOULDER_R,
    C_BTN_BACK,
    C_BTN_START,
    C_BTN_XBOX,
    C_BTN_THUMB_L,
    C_BTN_THUMB_R,

    C_STICK_L_X,
    C_STICK_L_Y,
    C_STICK_R_X,
    C_STICK_R_Y,
    C_TRIGGER_L,
    C_TRIGGER_R,
    C_DPAD_X,
    C_DPAD_Y,

    NONE,
};

// The deadzone of the axis controllers
const float DEADZONE = 0.1f;

// Stores a list of gamepad data
// For buttons: 0 is nothing, 1 is pressed, -1 is released.
// For axis: 32766 is maximum, -32767 is minimum
float state[JoystickInput::NONE];

/// @brief      Looks a specific Joystick input based on a type
/// @param      Type - The joystick type looking for
/// @returns    The correct joystick device name from in the system
const char* get_device_name (const JoystickType type) {
    // TODO
    return "/dev/input/js2";
}

/**
 * Reads a joystick event from the joystick device.
 *
 * Returns 0 on success. Otherwise -1 is returned.
 */
int read_event(int fd, struct js_event *event)
{
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));

    if (bytes == sizeof(*event))
        return 0;

    /* Error, could not read full event. */
    return -1;
}

/**
 * Returns the number of axes on the controller or 0 if an error occurs.
 */
size_t get_axis_count(int fd)
{
    __u8 axes;

    if (ioctl(fd, JSIOCGAXES, &axes) == -1)
        return 0;

    return axes;
}

/**
 * Returns the number of buttons on the controller or 0 if an error occurs.
 */
size_t get_button_count(int fd)
{
    __u8 buttons;
    if (ioctl(fd, JSIOCGBUTTONS, &buttons) == -1)
        return 0;

    return buttons;
}



/// @brief      Returns the Input action from a button pressed
/// @param      event - The reference to the Joystick event
/// @param      joystick - The current joystick type being used
/// @returns    The Joystick Input type for the button event
JoystickInput get_button (struct js_event *event, const JoystickType joystick) {
    if (joystick == JoystickType::CONTROLLER) {
        switch (event->number) {
            case 0: return C_BTN_A;
            case 1: return C_BTN_B;
            case 2: return C_BTN_X;
            case 3: return C_BTN_Y;
            case 4: return C_BTN_SHOULDER_L;
            case 5: return C_BTN_SHOULDER_R;
            case 6: return C_BTN_BACK;
            case 7: return C_BTN_START;
            case 8: return C_BTN_XBOX;
            case 9: return C_BTN_THUMB_L;
            case 10: return C_BTN_THUMB_R;
            default: return NONE;
        }
    }

    return NONE;
}

int get_button_value (struct js_event *event) {
    return event->value ? 1 : 0;
}

/// @brief      Returns the Input action from an axis event
/// @param      event - The reference to the Joystick event
/// @param      joystick - The current joystick type being used
/// @returns    The Joystick Input type for the axis event
JoystickInput get_axis (struct js_event *event, const JoystickType joystick) {
    if (joystick == JoystickType::CONTROLLER) {
        switch (event->number) {
            case 0: return C_STICK_L_X;
            case 1: return C_STICK_L_Y;
            case 2: return C_TRIGGER_L;
            case 3: return C_STICK_R_X;
            case 4: return C_STICK_R_Y;
            case 5: return C_TRIGGER_R;
            case 6: return C_DPAD_X;
            case 7: return C_DPAD_Y;
            default: return NONE;
        }
    }

    return NONE;
}

float get_axis_value (struct js_event *event, const JoystickInput input) {
    auto value = (float)event->value / 32767.0f;

    // Check for trigger inputs for remapping:
    if (input == JoystickInput::C_TRIGGER_L || input == JoystickInput::C_TRIGGER_R)
        value = (value + 1.0f) / 2.0f;
    else if (input == JoystickInput::C_STICK_L_Y || input == JoystickInput::C_STICK_R_Y || input == JoystickInput::C_DPAD_Y)
        value = -value;

    // Check for deadzone
    if (value < DEADZONE && value > -DEADZONE)
        return 0;
    else
        return value;
}

int main(int argc, char *argv[])
{
    const char *device;
    int joystick;
    struct js_event event;
    JoystickType type = JoystickType::CONTROLLER;
    JoystickInput button;
    JoystickInput axis;

    device = get_device_name(type);

    joystick = open(device, O_RDONLY);

    while (1) {
        bool valid = read_event(joystick, &event) == 0;
        if (valid) {
            switch (event.type) {
                case JS_EVENT_BUTTON:
                    button = get_button(&event, type);
                    state[button] = get_button_value(&event);
                    break;
                case JS_EVENT_AXIS:
                    axis = get_axis(&event, type);
                    state[axis] = get_axis_value(&event, axis);
                    break;
                default:
                    break;
            }

            // Print current state
            for (int i = 0; i < sizeof(state) / sizeof(int); i++) {
                cout << i << " : " << state[i] << endl;
            }
        }
        
        sleep(0.01);
        fflush(stdout);
        cout << endl;
    }
    /*

    if (js == -1)
        perror("Could not open joystick");

    // This loop will exit if the controller is unplugged.
    while (read_event(js, &event) == 0)
    {
        switch (event.type)
        {
            case JS_EVENT_BUTTON:
                printf("Button %u %s\n", event.number, event.value ? "pressed" : "released");
                break;
            case JS_EVENT_AXIS:
                axis = get_axis_state(&event, axes);
                if (axis < 10)
                    printf("Axis %zu at (%6d, %6d)\n", axis, axes[axis].x, axes[axis].y);
                break;
            default:
                printf("Hi");
                break;
        }
        
        fflush(stdout);
    }

    close(js);
    */
    return 0;
}