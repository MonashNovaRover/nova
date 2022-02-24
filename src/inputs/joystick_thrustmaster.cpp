/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "joystick_thrustmaster.h"


// Constructor for the thrustmaster joysticks
JoystickThrustmaster::JoystickThrustmaster(const bool left, const float offset)
    : Joystick((left) ? INPUT_THRUST_LEFT : INPUT_THRUST_RIGHT, offset) {
    first_message = construct_message_data();
    can_publish = false;
}


// Updates the message values for the joysticks
void JoystickThrustmaster::set_message_values() {

    // Checks if the gamepad is currently connected
    msg.connected = GamepadIsConnected(controller);

    // If the gamepad is connected
    if (msg.connected)
    {          
		msg = construct_message_data();
    }
	
	if (!can_publish && !compare_message_data(first_message)) {
		can_publish = true;
	}

    // If the input is not connected, lock the inputs and reset the message
    else
    {
        msg = core::msg::InputJoystick();
        twist_lock = true;
        hat_lock = true;
    }
}

core::msg::InputJoystick JoystickThrustmaster::construct_message_data()
{
    core::msg::InputJoystick new_msg;

    // Checks if the gamepad is currently connected
    new_msg.connected = GamepadIsConnected(controller);

    // Set the values in the ROS msg to the main stick
    new_msg.ax_stick_x = stick_lx_f;
    new_msg.ax_stick_y = stick_ly_f;
    new_msg.ax_stick_twist = -convert_trg2ax(GamepadTriggerLength(controller, TRIGGER_LEFT) + offset);
    if (new_msg.ax_stick_twist > -0.05 && new_msg.ax_stick_twist < 0.05) new_msg.ax_stick_twist = 0.0;
    if (new_msg.ax_stick_twist >= 0.99 - 2 * offset || new_msg.ax_stick_twist <= -0.99 - 2 * offset) new_msg.ax_stick_twist = sign(new_msg.ax_stick_twist);

    // Set the values in the ROS msg to thumb stick
    new_msg.ax_thumb_x = -stick_ry_f;
    new_msg.ax_thumb_y = to_int(-convert_trg2ax(GamepadTriggerLength(controller, TRIGGER_RIGHT)));

    // Set the values of the slider
    new_msg.ax_slider = convert_ax2trg(-stick_rx_f);

    // Set the state messages of each of the buttons
    new_msg.btn_thumb_l_state = get_button_state(BUTTON_X);
    new_msg.btn_thumb_r_state = get_button_state(BUTTON_Y);
    new_msg.btn_thumb_u_state = get_button_state(BUTTON_A);
    new_msg.btn_thumb_d_state = get_button_state(BUTTON_B);

    new_msg.btn_bottom_l1_state = get_button_state(BUTTON_LEFT_SHOULDER);
    new_msg.btn_bottom_l2_state = get_button_state(BUTTON_RIGHT_SHOULDER);
    new_msg.btn_bottom_l3_state = get_button_state(BUTTON_BACK);
    new_msg.btn_bottom_l4_state = get_button_state(BUTTON_LEFT_THUMB);
    new_msg.btn_bottom_l5_state = get_button_state(BUTTON_XBOX);
    new_msg.btn_bottom_l6_state = get_button_state(BUTTON_START);

    new_msg.btn_bottom_r1_state = get_button_state(BUTTON_EXTRA_2);
    new_msg.btn_bottom_r2_state = get_button_state(BUTTON_EXTRA_1);
    new_msg.btn_bottom_r3_state = get_button_state(BUTTON_RIGHT_THUMB);
    new_msg.btn_bottom_r4_state = get_button_state(BUTTON_EXTRA_3);
    new_msg.btn_bottom_r5_state = get_button_state(BUTTON_EXTRA_4);
    new_msg.btn_bottom_r6_state = get_button_state(BUTTON_EXTRA_5);

    return new_msg;
}

bool JoystickThrustmaster::compare_message_data(core::msg::InputJoystick other_msg){
    // Checking that every parameter is the same
    bool equals = true;
    // Set the values in the ROS msg to the main stick
    equals = (msg.ax_stick_x == other_msg.ax_stick_x);
    equals &= (msg.ax_stick_y == other_msg.ax_stick_y);
    equals &= (msg.ax_stick_twist == other_msg.ax_stick_twist);
    equals &= (msg.ax_thumb_x == other_msg.ax_thumb_x);
    equals &= (msg.ax_thumb_y == other_msg.ax_thumb_y);
    equals &= (msg.ax_slider == other_msg.ax_slider);
    equals &= (msg.btn_thumb_l_state == other_msg.btn_thumb_l_state);
    equals &= (msg.btn_thumb_r_state == other_msg.btn_thumb_r_state);
    equals &= (msg.btn_thumb_u_state == other_msg.btn_thumb_u_state);
    equals &= (msg.btn_thumb_d_state == other_msg.btn_thumb_d_state);
    equals &= (msg.btn_bottom_l1_state == other_msg.btn_bottom_l1_state);
    equals &= (msg.btn_bottom_l2_state == other_msg.btn_bottom_l2_state);
    equals &= (msg.btn_bottom_l3_state == other_msg.btn_bottom_l3_state);
    equals &= (msg.btn_bottom_l4_state == other_msg.btn_bottom_l4_state);
    equals &= (msg.btn_bottom_l5_state == other_msg.btn_bottom_l5_state);
    equals &= (msg.btn_bottom_l6_state == other_msg.btn_bottom_l6_state);
    equals &= (msg.btn_bottom_r1_state == other_msg.btn_bottom_r1_state);
    equals &= (msg.btn_bottom_r2_state == other_msg.btn_bottom_r2_state);
    equals &= (msg.btn_bottom_r3_state == other_msg.btn_bottom_r3_state);
    equals &= (msg.btn_bottom_r4_state == other_msg.btn_bottom_r4_state);
    equals &= (msg.btn_bottom_r5_state == other_msg.btn_bottom_r5_state);
    equals &= (msg.btn_bottom_r6_state == other_msg.btn_bottom_r6_state);
    return equals;
}


// Returns the input joystick message object
core::msg::InputJoystick JoystickThrustmaster::get_message() {
    if (can_publish)
    {
        return msg;
    }
    else 
    {
        return core::msg::InputJoystick();
    }
}


