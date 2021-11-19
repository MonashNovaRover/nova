/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "joystick_thrustmaster.h"


JoystickThrustmaster::JoystickThrustmaster(const bool left, const float offset)
    : Joystick((left) ? INPUT_THRUST_LEFT : INPUT_THRUST_RIGHT, offset) {

}

/*
    Fetches rest of controller values and updates the message object
*/
void JoystickThrustmaster::set_message_values() {

    // Checks if the gamepad is currently connected
    msg.connected = GamepadIsConnected(controller);

    // If the gamepad is connected
    if (msg.connected)
    {          
        // Set the values in the ROS msg to the main stick
        msg.ax_stick_x = stick_lx_f;
        msg.ax_stick_y = stick_ly_f;
        msg.ax_stick_twist = convert_trg2ax(GamepadTriggerLength(controller, TRIGGER_LEFT));

        // Set the values in the ROS msg to thumb stick
        msg.ax_thumb_x = -stick_ry_f;
        msg.ax_thumb_y = -convert_trg2ax(GamepadTriggerLength(controller, TRIGGER_RIGHT));

        // Set the values of the slider
        msg.ax_slider = convert_ax2trg(-stick_rx_f);

        // Set the state messages of each of the buttons
        msg.btn_thumb_l_state = get_button_state(BUTTON_X);
        msg.btn_thumb_r_state = get_button_state(BUTTON_Y);
        msg.btn_thumb_u_state = get_button_state(BUTTON_A);
        msg.btn_thumb_d_state = get_button_state(BUTTON_B);

        msg.btn_bottom_l1_state = get_button_state(BUTTON_RIGHT_THUMB);
        msg.btn_bottom_l2_state = get_button_state(BUTTON_DPAD_UP);
        msg.btn_bottom_l3_state = 0;
        msg.btn_bottom_l4_state = 0;
        msg.btn_bottom_l5_state = 0;
        msg.btn_bottom_l6_state = 0;

        msg.btn_bottom_r1_state = get_button_state(BUTTON_BACK);
        msg.btn_bottom_r2_state = get_button_state(BUTTON_RIGHT_SHOULDER);
        msg.btn_bottom_r3_state = get_button_state(BUTTON_LEFT_SHOULDER);
        msg.btn_bottom_r4_state = get_button_state(BUTTON_START);
        msg.btn_bottom_r5_state = get_button_state(BUTTON_XBOX);
        msg.btn_bottom_r6_state = get_button_state(BUTTON_LEFT_THUMB);

        return;

        /*
        // Left trigger
        if (twist_lock && GamepadTriggerLength(controller, TRIGGER_LEFT) < 0.1)
            msg.trg_l_val = 0.0;

        else
        {
            msg.trg_l_val = GamepadTriggerLength(controller, TRIGGER_LEFT) - offset;
            msg.trg_l_val = (msg.trg_l_val > 0.0) ? msg.trg_l_val/(1 - offset): msg.trg_l_val/(offset);
          
            // Look for invalid input
            if ((msg.trg_l_val < 0.01 && msg.trg_l_val > -0.01) || isnan(msg.trg_l_val))
                msg.trg_l_val = 0.0;

            twist_lock = false;
        }

        // Right trigger
        if (hat_lock && GamepadTriggerLength(controller, TRIGGER_RIGHT) < 0.1)
            msg.trg_r_val = 0.0;

        else
        {
            msg.trg_r_val = GamepadTriggerLength(controller, TRIGGER_RIGHT)-offset;
            msg.trg_r_val = (msg.trg_r_val>0.0) ? msg.trg_r_val/(1-offset): msg.trg_r_val/(offset); //Re-scale OFFSET value
            
            // Look for invalid input
            if ((msg.trg_r_val < 0.01 && msg.trg_r_val > -0.01) || isnan(msg.trg_r_val)) // Get rid of tiny floats
                msg.trg_r_val = 0.0;
            
            hat_lock = false;
        }
        */
    }

    // If the input is not connected, lock the inputs and reset the message
    else
    {
        msg = core::msg::InputJoystick();
        twist_lock = true;
        hat_lock = true;
    }
}

/*
    Returns the message object from the instance
*/
core::msg::InputJoystick JoystickThrustmaster::get_message() {
    return msg;
}


