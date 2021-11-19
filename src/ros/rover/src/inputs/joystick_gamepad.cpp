/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "joystick_gamepad.h"

JoystickGamepad::JoystickGamepad(const float offset)
    : Joystick(INPUT_XBOX, offset) {

}

/*
    Fetches rest of controller values and updates the message object
*/
void JoystickGamepad::set_message_values() {

    // Checks if the gamepad is currently connected
    msg.connected = GamepadIsConnected(controller);

    // If the gamepad is connected
    if (msg.connected)
    {          
        // Set the values in the ROS msg to the axis values
        msg.ax_stick_l_x = stick_lx_f; 
        msg.ax_stick_l_y = stick_ly_f;   
        msg.ax_stick_r_x = stick_rx_f; 
        msg.ax_stick_r_y = stick_ry_f;

        // Set the state messages of each of the buttons
        msg.btn_a_state = get_button_state(BUTTON_A);
        msg.btn_b_state = get_button_state(BUTTON_B);
        msg.btn_x_state = get_button_state(BUTTON_X);
        msg.btn_y_state = get_button_state(BUTTON_Y);
        msg.btn_start_state = get_button_state(BUTTON_START);
        msg.btn_back_state = get_button_state(BUTTON_BACK);
        msg.btn_shoulder_l_state = get_button_state(BUTTON_LEFT_SHOULDER);
        msg.btn_shoulder_r_state = get_button_state(BUTTON_RIGHT_SHOULDER);
        msg.btn_xbox_state = get_button_state(BUTTON_XBOX);
        msg.btn_thumb_l_state = get_button_state(BUTTON_LEFT_THUMB);
        msg.btn_thumb_r_state = get_button_state(BUTTON_RIGHT_THUMB);
        msg.btn_dpad_l_state = get_button_state(BUTTON_DPAD_LEFT);
        msg.btn_dpad_r_state = get_button_state(BUTTON_DPAD_RIGHT );
        msg.btn_dpad_u_state = get_button_state(BUTTON_DPAD_UP );
        msg.btn_dpad_d_state = get_button_state(BUTTON_DPAD_DOWN );

        
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

        // When the joystick is first connected the twist and hat give a 0.0 until moved, whereas their actual centre is 0.435. This ensures that they have been moved first so they don't make a full negative power to twist and hat on connection
        msg.btn_trigger_l_down = GamepadTriggerDown(controller, TRIGGER_LEFT);
        msg.btn_trigger_r_down = GamepadTriggerDown(controller, TRIGGER_RIGHT);

        // Get DPAD inputs
        bool dpad_l = GamepadButtonDown(controller, BUTTON_DPAD_LEFT);
        bool dpad_r = GamepadButtonDown(controller, BUTTON_DPAD_RIGHT);
        bool dpad_u = GamepadButtonDown(controller, BUTTON_DPAD_UP);
        bool dpad_d = GamepadButtonDown(controller, BUTTON_DPAD_DOWN);

        // Calculate the DPAD data
        msg.ax_dpad_x = dpad_r - dpad_l; // Left -1, none/both 0, right 1
        msg.ax_dpad_y = dpad_u - dpad_d; // Down -1, none/both 0, up 1
        
    }

    // If the input is not connected, lock the inputs and reset the message
    else
    {
        msg = core::msg::InputGamepad();
        twist_lock = true;
        hat_lock = true;
    }
}

/*
    Returns the message object from the instance
*/
core::msg::InputGamepad JoystickGamepad::get_message() {
    return msg;
}
