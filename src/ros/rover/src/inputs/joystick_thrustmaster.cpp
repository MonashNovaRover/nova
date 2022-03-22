/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "joystick_thrustmaster.h"
#include <iostream>


// Constructor for the thrustmaster joysticks
JoystickThrustmaster::JoystickThrustmaster(const bool left, const float offset, const float default_slider)
    : Joystick((left) ? INPUT_THRUST_LEFT : INPUT_THRUST_RIGHT, offset) {
    default_ax_slider = default_slider;
}

// Checks for valid inputs
void JoystickThrustmaster::check_valid_inputs() {
    // Don't perform the checks if valid already
    if (valid_ax_stick_twist && valid_ax_thumb_x && valid_ax_slider) return;

    // Check for stick twisting
    if (!valid_ax_stick_twist) {
        valid_ax_stick_twist = -convert_trg2ax(GamepadTriggerLength(controller, TRIGGER_LEFT) + offset) < 0.5;
    }

    // Check for thumb axis
    if (!valid_ax_thumb_x) {
        valid_ax_thumb_x = to_int(-convert_trg2ax(GamepadTriggerLength(controller, TRIGGER_RIGHT))) == 0.0;
    }

    // Check for slider axis
    if (!valid_ax_slider) {
        valid_ax_slider = convert_ax2trg(-stick_rx_f) != 0.5;
    }
}


// Updates the message values for the joysticks
void JoystickThrustmaster::set_message_values() {

    // Checks if the gamepad is currently connected
    msg.connected = GamepadIsConnected(controller);

    // If the gamepad is connected
    if (msg.connected)
    {          
        // Check for valid inputs
        check_valid_inputs();

        // Set the values in the ROS msg to the main stick
        // Use the same coordinate system as the arm base coordinates
        // Forward on joystick -> +ve x
        // Left on joystick -> +ve y
        // Anticlockwise twist -> +ve z
        msg.ax_stick_x = stick_ly_f;
        msg.ax_stick_y = -stick_lx_f;

        // Make sure the ax twist is valid
        if (valid_ax_stick_twist) {
            msg.ax_stick_twist = -convert_trg2ax(GamepadTriggerLength(controller, TRIGGER_LEFT) + offset);
            if (msg.ax_stick_twist > -0.05 && msg.ax_stick_twist < 0.05) msg.ax_stick_twist = 0.0;
            if (msg.ax_stick_twist >= 0.99 - 2 * offset || msg.ax_stick_twist <= -0.99 - 2 * offset) msg.ax_stick_twist = sign(msg.ax_stick_twist);
        } else
            msg.ax_stick_twist = 0.0;

        // Set the values in the ROS msg to thumb stick
        // Use the same coordinate system as the arm base coordinates
        if (valid_ax_thumb_x)
            msg.ax_thumb_x = to_int(-convert_trg2ax(GamepadTriggerLength(controller, TRIGGER_RIGHT)));
        else
            msg.ax_thumb_x = 0.0;
        msg.ax_thumb_y = stick_ry_f;

        // Set the values of the slider
        if (valid_ax_slider)
            msg.ax_slider = convert_ax2trg(-stick_rx_f);
        else
            msg.ax_slider = default_ax_slider;

        // Set the state messages of each of the buttons
        msg.btn_thumb_l_state = get_button_state(BUTTON_X);
        msg.btn_thumb_r_state = get_button_state(BUTTON_Y);
        msg.btn_thumb_u_state = get_button_state(BUTTON_A);
        msg.btn_thumb_d_state = get_button_state(BUTTON_B);

        msg.btn_bottom_l1_state = get_button_state(BUTTON_LEFT_SHOULDER);
        msg.btn_bottom_l2_state = get_button_state(BUTTON_RIGHT_SHOULDER);
        msg.btn_bottom_l3_state = get_button_state(BUTTON_BACK);
        msg.btn_bottom_l4_state = get_button_state(BUTTON_LEFT_THUMB);
        msg.btn_bottom_l5_state = get_button_state(BUTTON_XBOX);
        msg.btn_bottom_l6_state = get_button_state(BUTTON_START);

        msg.btn_bottom_r1_state = get_button_state(BUTTON_EXTRA_2);
        msg.btn_bottom_r2_state = get_button_state(BUTTON_EXTRA_1);
        msg.btn_bottom_r3_state = get_button_state(BUTTON_RIGHT_THUMB);
        msg.btn_bottom_r4_state = get_button_state(BUTTON_EXTRA_3);
        msg.btn_bottom_r5_state = get_button_state(BUTTON_EXTRA_4);
        msg.btn_bottom_r6_state = get_button_state(BUTTON_EXTRA_5);
    }

    // If the input is not connected, lock the inputs and reset the message
    else
    {
        msg = core::msg::InputJoystick();
        twist_lock = true;
        hat_lock = true;
    }
}


// Returns the input joystick message object
core::msg::InputJoystick JoystickThrustmaster::get_message() {
    return msg;
}

// Resets the joystick inputs
void JoystickThrustmaster::reset_inputs () {
    valid_ax_stick_twist = false;
    valid_ax_thumb_x = false;
    valid_ax_slider = false;
}
