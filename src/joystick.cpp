/*
 *--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--
 * A joystick class to handle xbox/arm joystick inputs 
 *
 * Author: Marcel Masque (marcel.masques@gmail.com)
 *
 * Date last updated: 29/01/20 by Marcel
 *--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
 */
#include "joystick.h"

/*
 *--**--..--**----**--..--**--..--**--..--**--..--**--..--**--..--**--
 * Constructor used when offset is needed
 *    Initialises controller position values, offsets and controller boolean settings
 *    Initialises controller instance 
 *--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
 */
Joystick::Joystick(const GAMEPAD_DEVICE controller, const float offset) {


    this->offset = offset;

    stick_lx = 0.0;
    stick_ly = 0.0;
    stick_rx = 0.0;
    stick_ry = 0.0;
    twist_lock = true;
    hat_lock = true;
    this->controller = controller;
}
/*
 *--**--..--**----**--..--**--..--**--..--**--..--**--..--**--..--**--
 * Constructor used when offset is NOT  needed
 *    Initialises controller position values, controller boolean settings
 *    Initialises controller instance 
 *--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
 */
Joystick::Joystick(const GAMEPAD_DEVICE controller) {

	stick_lx = 0.0;
	stick_ly = 0.0;
	stick_rx = 0.0;
	stick_ry = 0.0;
    offset = 0;
    twist_lock = true;
    hat_lock = true;
    this->controller = controller;
}
/*
 *--**--..--**----**--..--**--..--**--..--**--..--**--..--**--..--**--
 * Fetches stick values, corrects for deadzone and sets message values.
 *--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
 */
void Joystick::Update() {

	// grab stick values
    GamepadStickXY(controller, STICK_LEFT, &stick_lx, &stick_ly);
    GamepadStickXY(controller, STICK_RIGHT, &stick_rx, &stick_ry);
    
	// correct for deadzone
    CorrectDeadzone();
	// set all message values
    SetMessageValues();
}
/*
 *--**--..--**----**--..--**--..--**--..--**--..--**--..--**--..--**--
 * The gamepad sticks have a deadzone - which means for a small amount of
 * movement of the stick, the reading remains at zero. This means as soon
 * as you move the stick out of the deadzone, the reading will jump from 
 * zero to some higher value. The calculations here account for this and 
 * rescale the values to remove this jump.
 *--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
 */
void Joystick::CorrectDeadzone() {

    
    stick_lx_f = Sign(stick_lx)*((float) abs(stick_lx) - GAMEPAD_DEADZONE_LEFT_STICK)/STICK_MAX_L;
    stick_ly_f = Sign(stick_ly)*((float) abs(stick_ly) - GAMEPAD_DEADZONE_LEFT_STICK)/STICK_MAX_L;

    stick_rx_f = Sign(stick_rx)*((float) abs(stick_rx) - GAMEPAD_DEADZONE_RIGHT_STICK)/STICK_MAX_R;
    stick_ry_f = Sign(stick_ry)*((float) abs(stick_ry) - GAMEPAD_DEADZONE_RIGHT_STICK)/STICK_MAX_R;
    
    
}

int Joystick::GetButtonState (const GAMEPAD_BUTTON button) {
    if (GamepadButtonTriggered(controller, button))
        return 1;
    else if (GamepadButtonReleased(controller, button))
        return 3;
    else if (GamepadButtonDown(controller, button))
        return 2;
    
    return 0;
}

/*
 *--**--..--**----**--..--**--..--**--..--**--..--**--..--**--..--**--
 * Fetches rest of controller values and updates the message object
 *--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
 */
void Joystick::SetMessageValues() {

    msg.connected = GamepadIsConnected(controller); // Check controller connection
    if (msg.connected)
    {   
        
        // Set the values in the ROS msg
        msg.ax_stick_l_x = stick_lx_f; 
        msg.ax_stick_l_y = stick_ly_f;   
        msg.ax_stick_r_x = stick_rx_f; 
        msg.ax_stick_r_y = stick_ry_f;

        // Set the state messages
        msg.btn_a_state = GetButtonState(BUTTON_A);
        msg.btn_b_state = GetButtonState(BUTTON_B);
        msg.btn_x_state = GetButtonState(BUTTON_X);
        msg.btn_y_state = GetButtonState(BUTTON_Y);
        msg.btn_start_state = GetButtonState(BUTTON_START);
        msg.btn_back_state = GetButtonState(BUTTON_BACK);
        msg.btn_shoulder_l_state = GetButtonState(BUTTON_LEFT_SHOULDER);
        msg.btn_shoulder_r_state = GetButtonState(BUTTON_RIGHT_SHOULDER);
        msg.btn_xbox_state = GetButtonState(BUTTON_XBOX);
        msg.btn_thumb_l_state = GetButtonState(BUTTON_LEFT_THUMB);
        msg.btn_thumb_r_state = GetButtonState(BUTTON_RIGHT_THUMB);
        msg.btn_dpad_l_state = GetButtonState(BUTTON_DPAD_LEFT);
        msg.btn_dpad_r_state = GetButtonState(BUTTON_DPAD_RIGHT );
        msg.btn_dpad_u_state = GetButtonState(BUTTON_DPAD_UP );
        msg.btn_dpad_d_state = GetButtonState(BUTTON_DPAD_DOWN );

        
        //left
        if (twist_lock and GamepadTriggerLength(controller, TRIGGER_LEFT) < 0.1)
        {
            msg.trg_l_val = 0.0;
        }
        else
        {
            msg.trg_l_val = GamepadTriggerLength(controller, TRIGGER_LEFT) - offset;
            msg.trg_l_val = (msg.trg_l_val > 0.0) ? msg.trg_l_val/(1 - offset): msg.trg_l_val/(offset);
          
            if ((msg.trg_l_val < 0.01 && msg.trg_l_val > -0.01) || isnan(msg.trg_l_val))
            {
                msg.trg_l_val = 0.0;
            }

            twist_lock = false;
        }
        // right
        if (hat_lock and GamepadTriggerLength(controller, TRIGGER_RIGHT) < 0.1)
        {
            msg.trg_r_val = 0.0;
        }
        else
        {
            msg.trg_r_val = GamepadTriggerLength(controller, TRIGGER_RIGHT)-offset;
            msg.trg_r_val = (msg.trg_r_val>0.0) ? msg.trg_r_val/(1-offset): msg.trg_r_val/(offset); //Re-scale OFFSET value
            if ((msg.trg_r_val < 0.01 && msg.trg_r_val > -0.01) || isnan(msg.trg_r_val)) // Get rid of tiny floats
            { 
                msg.trg_r_val = 0.0;
            }
            hat_lock = false;
        }

        //When the joystick is first connected the twist and hat give a 0.0 until moved, whereas their actual centre is 0.435. This ensures that they have been moved first so they don't make a full negative power to twist and hat on connection
        msg.btn_trigger_l_down = GamepadTriggerDown(controller, TRIGGER_LEFT);
        msg.btn_trigger_r_down = GamepadTriggerDown(controller, TRIGGER_RIGHT);

        bool dpad_l = GamepadButtonDown(controller, BUTTON_DPAD_LEFT);
        bool dpad_r = GamepadButtonDown(controller, BUTTON_DPAD_RIGHT);

        bool dpad_u = GamepadButtonDown(controller, BUTTON_DPAD_UP);
        bool dpad_d = GamepadButtonDown(controller, BUTTON_DPAD_DOWN);

        msg.ax_dpad_x = dpad_r - dpad_l; // Left -1, none/both 0, right 1
        msg.ax_dpad_y = dpad_u - dpad_d; // Down -1, none/both 0, up 1
        
    }
    else
    {
        msg.ax_stick_l_x = 0.0;
        msg.ax_stick_l_y = 0.0;
        msg.ax_stick_r_x = 0.0;
        msg.ax_stick_r_y = 0.0;
        twist_lock = true;
        hat_lock = true;
    }
}

core::msg::InputGamepad Joystick::GetMessage() {
    return msg;
}

//--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--
// sgn():
//
//    Returns the sign of the input float (-1, 0 or 1).
//--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
int Joystick::Sign(const float val) {
    return (int)(0.0 < val) - (val < 0.0);
}