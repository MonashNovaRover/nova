/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Marcel Masque, Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "joystick.h"


// Base constructor that takes in some input and an offset
Joystick::Joystick(const InputType input, const float offset) {

    // Set the controller and offset instance variables
    this->offset = offset;

    // Set the controller based on the input type
    if (input == INPUT_XBOX)
        this->controller = GAMEPAD_0;
    else if (input == INPUT_THRUST_LEFT)
        this->controller = GAMEPAD_1;
    else if (input == INPUT_THRUST_RIGHT)
        this->controller = GAMEPAD_2;

    // Reset the axis inputs
    stick_lx = 0.0;
    stick_ly = 0.0;
    stick_rx = 0.0;
    stick_ry = 0.0;
    twist_lock = true;
    hat_lock = true;
}


// Updates the gamepad and corrects for a deadzone
void Joystick::update() {

	// grab stick values
    GamepadStickXY(controller, STICK_LEFT, &stick_lx, &stick_ly);
    GamepadStickXY(controller, STICK_RIGHT, &stick_rx, &stick_ry);
    
	// correct for deadzone
    correct_deadzone();

    // Set the message values from the message objects
    set_message_values();
}


// Overriden by base classes to update the message data
void Joystick::set_message_values() {

}


// The gamepad sticks have a deadzone - which means for a small amount of
//    movement of the stick, the reading remains at zero. This means as soon
//    as the stick moves out of the deadzone, the reading will jump from 
//    zero to some higher value. The calculations here account for this and 
//    rescale the values to remove this jump.
void Joystick::correct_deadzone() {

    // Updates the left stick float values
    stick_lx_f = sign(stick_lx) * ((float) abs(stick_lx) - GAMEPAD_DEADZONE_LEFT_STICK) / STICK_MAX_L;
    stick_ly_f = sign(stick_ly) * ((float) abs(stick_ly) - GAMEPAD_DEADZONE_LEFT_STICK) / STICK_MAX_L;

    // Updates the right stick float values
    stick_rx_f = sign(stick_rx) * ((float) abs(stick_rx) - GAMEPAD_DEADZONE_RIGHT_STICK) / STICK_MAX_R;
    stick_ry_f = sign(stick_ry) * ((float) abs(stick_ry) - GAMEPAD_DEADZONE_RIGHT_STICK) / STICK_MAX_R;
}


// Returns the state of a button input based on how it is pressed
int Joystick::get_button_state (const GAMEPAD_BUTTON button) {
    if (GamepadButtonTriggered(controller, button))
        return 1;
    else if (GamepadButtonReleased(controller, button))
        return 3;
    else if (GamepadButtonDown(controller, button))
        return 2;
    
    // No input for this button
    return 0;
}


// Returns the sign of the input float (-1 or 1).
int Joystick::sign(const float val) {
    return (int)(val > 0.0) - (int)(val < 0.0);
}


// (0 to 1) to (-1 to 1)
float Joystick::convert_trg2ax (const float val) {
    return val * 2.0 - 1.0;
}


// (-1 to 1) to (0 to 1)
float Joystick::convert_ax2trg (const float val) {
    return (val + 1.0) / 2.0;
}


// Converts a float to an integer float
float Joystick::to_int (const float val) {
    if (val >= 1.0)
        return 1.0;
    else if (val <= -1.0)
        return -1.0;
    else
        return 0.0;
}