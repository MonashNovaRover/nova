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
Joystick::Joystick(GAMEPAD_DEVICE controller, float offset) {


    offset_ = offset;

    stick_lx_ = 0.0;
    stick_ly_ = 0.0;
    stick_rx_ = 0.0;
    stick_ry_ = 0.0;
    twist_lock_ = true;
    hat_lock_ = true;
    controller_ = controller;
}
/*
 *--**--..--**----**--..--**--..--**--..--**--..--**--..--**--..--**--
 * Constructor used when offset is NOT  needed
 *    Initialises controller position values, controller boolean settings
 *    Initialises controller instance 
 *--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
 */
Joystick::Joystick(GAMEPAD_DEVICE controller) {

	stick_lx_ = 0.0;
	stick_ly_ = 0.0;
	stick_rx_ = 0.0;
	stick_ry_ = 0.0;
    offset_ = 0;
    twist_lock_ = true;
    hat_lock_ = true;
    controller_ = controller;
}
/*
 *--**--..--**----**--..--**--..--**--..--**--..--**--..--**--..--**--
 * Fetches stick values, corrects for deadzone and sets message values.
 *--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
 */
void Joystick::update() {

	// grab stick values
    GamepadStickXY(controller_, STICK_LEFT, &stick_lx_, &stick_ly_);
    GamepadStickXY(controller_, STICK_RIGHT, &stick_rx_, &stick_ry_);
    
	// correct for deadzone
    correctForDeadzone();
	// set all message values
    setMessageValues();
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
void Joystick::correctForDeadzone() {

    
    stick_lx_f = sgn(stick_lx_)*((float) abs(stick_lx_) - GAMEPAD_DEADZONE_LEFT_STICK)/STICK_MAX_L_;
    stick_ly_f = sgn(stick_ly_)*((float) abs(stick_ly_) - GAMEPAD_DEADZONE_LEFT_STICK)/STICK_MAX_L_;

    stick_rx_f = sgn(stick_rx_)*((float) abs(stick_rx_) - GAMEPAD_DEADZONE_RIGHT_STICK)/STICK_MAX_R_;
    stick_ry_f = sgn(stick_ry_)*((float) abs(stick_ry_) - GAMEPAD_DEADZONE_RIGHT_STICK)/STICK_MAX_R_;
    
    
}

int Joystick::GetButtonState (const GAMEPAD_BUTTON button) {
    if (GamepadButtonTriggered(controller_, button))
        return 1;
    else if (GamepadButtonReleased(controller_, button))
        return 3;
    else if (GamepadButtonDown(controller_, button))
        return 2;
    
    return 0;
}

/*
 *--**--..--**----**--..--**--..--**--..--**--..--**--..--**--..--**--
 * Fetches rest of controller values and updates the message object
 *--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
 */
void Joystick::setMessageValues() {

    msg_.connected = GamepadIsConnected(controller_); // Check controller connection
    if (msg_.connected)
    {   
        
        // Set the values in the ROS msg_
        msg_.ax_stick_l_x = stick_lx_f; 
        msg_.ax_stick_l_y = stick_ly_f;   
        msg_.ax_stick_r_x = stick_rx_f; 
        msg_.ax_stick_r_y = stick_ry_f;   

        // Set the button messages
        /*
        msg_.btn_a_down = GamepadButtonDown(controller_, BUTTON_A);
        msg_.btn_b_down = GamepadButtonDown(controller_, BUTTON_B);
        msg_.btn_x_down = GamepadButtonDown(controller_, BUTTON_X);
        msg_.btn_y_down = GamepadButtonDown(controller_, BUTTON_Y);
        msg_.btn_start_down = GamepadButtonDown(controller_, BUTTON_START);
        msg_.btn_back_down = GamepadButtonDown(controller_, BUTTON_BACK);
        msg_.btn_shoulder_l_down = GamepadButtonDown(controller_, BUTTON_LEFT_SHOULDER);
        msg_.btn_shoulder_r_down = GamepadButtonDown(controller_, BUTTON_RIGHT_SHOULDER);
        msg_.btn_xbox_down = GamepadButtonDown(controller_, BUTTON_XBOX );
        msg_.btn_thumb_l_down = GamepadButtonDown(controller_, BUTTON_LEFT_THUMB );
        msg_.btn_thumb_r_down = GamepadButtonDown(controller_, BUTTON_RIGHT_THUMB );
        msg_.btn_dpad_l_down = GamepadButtonDown(controller_, BUTTON_DPAD_LEFT);
        msg_.btn_dpad_r_down = GamepadButtonDown(controller_, BUTTON_DPAD_RIGHT );
        msg_.btn_dpad_u_down = GamepadButtonDown(controller_, BUTTON_DPAD_UP );
        msg_.btn_dpad_d_down = GamepadButtonDown(controller_, BUTTON_DPAD_DOWN );
        */

        // Set the state messages
        msg_.btn_a_state = GetButtonState(BUTTON_A);
        msg_.btn_b_state = GetButtonState(BUTTON_B);
        msg_.btn_x_state = GetButtonState(BUTTON_X);
        msg_.btn_y_state = GetButtonState(BUTTON_Y);
        msg_.btn_start_state = GetButtonState(BUTTON_START);
        msg_.btn_back_state = GetButtonState(BUTTON_BACK);
        msg_.btn_shoulder_l_state = GetButtonState(BUTTON_LEFT_SHOULDER);
        msg_.btn_shoulder_r_state = GetButtonState(BUTTON_RIGHT_SHOULDER);
        msg_.btn_xbox_state = GetButtonState(BUTTON_XBOX);
        msg_.btn_thumb_l_state = GetButtonState(BUTTON_LEFT_THUMB);
        msg_.btn_thumb_r_state = GetButtonState(BUTTON_RIGHT_THUMB);
        msg_.btn_dpad_l_state = GetButtonState(BUTTON_DPAD_LEFT);
        msg_.btn_dpad_r_state = GetButtonState(BUTTON_DPAD_RIGHT );
        msg_.btn_dpad_u_state = GetButtonState(BUTTON_DPAD_UP );
        msg_.btn_dpad_d_state = GetButtonState(BUTTON_DPAD_DOWN );

        
        //left
        if (twist_lock_ and GamepadTriggerLength(controller_, TRIGGER_LEFT) < 0.1)
        {
            msg_.trg_l_val = 0.0;
        }
        else
        {
            msg_.trg_l_val = GamepadTriggerLength(controller_, TRIGGER_LEFT) - offset_;
            msg_.trg_l_val = (msg_.trg_l_val > 0.0) ? msg_.trg_l_val/(1 - offset_): msg_.trg_l_val/(offset_);
          
            if ((msg_.trg_l_val < 0.01 && msg_.trg_l_val > -0.01) || isnan(msg_.trg_l_val))
            {
                msg_.trg_l_val = 0.0;
            }

            twist_lock_ = false;
        }
        // right
        if (hat_lock_ and GamepadTriggerLength(controller_, TRIGGER_RIGHT) < 0.1)
        {
            msg_.trg_r_val = 0.0;
        }
        else
        {
            msg_.trg_r_val = GamepadTriggerLength(controller_, TRIGGER_RIGHT)-offset_;
            msg_.trg_r_val = (msg_.trg_r_val>0.0) ? msg_.trg_r_val/(1-offset_): msg_.trg_r_val/(offset_); //Re-scale OFFSET value
            if ((msg_.trg_r_val < 0.01 && msg_.trg_r_val > -0.01) || isnan(msg_.trg_r_val)) // Get rid of tiny floats
            { 
                msg_.trg_r_val = 0.0;
            }
            hat_lock_ = false;
        }

        //When the joystick is first connected the twist and hat give a 0.0 until moved, whereas their actual centre is 0.435. This ensures that they have been moved first so they don't make a full negative power to twist and hat on connection
        msg_.btn_trigger_l_down = GamepadTriggerDown(controller_, TRIGGER_LEFT);
        msg_.btn_trigger_r_down = GamepadTriggerDown(controller_, TRIGGER_RIGHT);

        bool dpad_l = GamepadButtonDown(controller_, BUTTON_DPAD_LEFT);
        bool dpad_r = GamepadButtonDown(controller_, BUTTON_DPAD_RIGHT);

        bool dpad_u = GamepadButtonDown(controller_, BUTTON_DPAD_UP);
        bool dpad_d = GamepadButtonDown(controller_, BUTTON_DPAD_DOWN);

        msg_.ax_dpad_x = dpad_r - dpad_l; // Left -1, none/both 0, right 1
        msg_.ax_dpad_y = dpad_u - dpad_d; // Down -1, none/both 0, up 1
        
    }
    else
    {
        msg_.ax_stick_l_x = 0.0;
        msg_.ax_stick_l_y = 0.0;
        msg_.ax_stick_r_x = 0.0;
        msg_.ax_stick_r_y = 0.0;
        twist_lock_ = true;
        hat_lock_ = true;
    }
}

core::msg::InputGamepad Joystick::getMessage() {
    return msg_;
}

//--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--
// sgn():
//
//    Returns the sign of the input float (-1, 0 or 1).
//--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
int Joystick::sgn(float val) {
    return (int)(0.0 < val) - (val < 0.0);
}