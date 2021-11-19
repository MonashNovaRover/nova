/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Marcel Masque, Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "joystick.h"


/*
    Constructor used when offset is needed
    Initialises controller position values, offsets and controller boolean settings
    Initialises controller instance 
*/
Joystick::Joystick(const InputType input, const float offset) {

    // Set the controller and offset instance variables
    this->offset = offset;
    this->type = input;

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


/*
    Constructor used when offset is NOT  needed
    Initialises controller position values, controller boolean settings
    Initialises controller instance 
*/
Joystick::Joystick(const InputType input) : 
    Joystick(input, 0.0) {
}


/*
    Fetches stick values, corrects for deadzone and sets message values.
*/
void Joystick::update() {

	// grab stick values
    GamepadStickXY(controller, STICK_LEFT, &stick_lx, &stick_ly);
    GamepadStickXY(controller, STICK_RIGHT, &stick_rx, &stick_ry);
    
	// correct for deadzone
    correct_deadzone();

	// set all message values
    // TODO select
    if (type == INPUT_XBOX)
        set_message_values_gamepad();
    else if (type == INPUT_THRUST_LEFT || type == INPUT_THRUST_RIGHT)
        set_message_values_joystick();
}


/*
    The gamepad sticks have a deadzone - which means for a small amount of
    movement of the stick, the reading remains at zero. This means as soon
    as you move the stick out of the deadzone, the reading will jump from 
    zero to some higher value. The calculations here account for this and 
    rescale the values to remove this jump.
*/
void Joystick::correct_deadzone() {

    // Updates the left stick float values
    stick_lx_f = sign(stick_lx) * ((float) abs(stick_lx) - GAMEPAD_DEADZONE_LEFT_STICK) / STICK_MAX_L;
    stick_ly_f = sign(stick_ly) * ((float) abs(stick_ly) - GAMEPAD_DEADZONE_LEFT_STICK) / STICK_MAX_L;

    // Updates the right stick float values
    stick_rx_f = sign(stick_rx) * ((float) abs(stick_rx) - GAMEPAD_DEADZONE_RIGHT_STICK) / STICK_MAX_R;
    stick_ry_f = sign(stick_ry) * ((float) abs(stick_ry) - GAMEPAD_DEADZONE_RIGHT_STICK) / STICK_MAX_R;
}


/*
    Returns the state of a particular button input based on how the button
    has been pressed. If no input, it will be a 0. If triggered, 1. If the
    button has been held, then 2 and when the button is released, 3.
*/
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


/*
    Fetches rest of controller values and updates the message object
*/
void Joystick::set_message_values_gamepad() {

    // Checks if the gamepad is currently connected
    msg_gamepad.connected = GamepadIsConnected(controller);

    // If the gamepad is connected
    if (msg_gamepad.connected)
    {          
        // Set the values in the ROS msg to the axis values
        msg_gamepad.ax_stick_l_x = stick_lx_f; 
        msg_gamepad.ax_stick_l_y = stick_ly_f;   
        msg_gamepad.ax_stick_r_x = stick_rx_f; 
        msg_gamepad.ax_stick_r_y = stick_ry_f;

        // Set the state messages of each of the buttons
        msg_gamepad.btn_a_state = get_button_state(BUTTON_A);
        msg_gamepad.btn_b_state = get_button_state(BUTTON_B);
        msg_gamepad.btn_x_state = get_button_state(BUTTON_X);
        msg_gamepad.btn_y_state = get_button_state(BUTTON_Y);
        msg_gamepad.btn_start_state = get_button_state(BUTTON_START);
        msg_gamepad.btn_back_state = get_button_state(BUTTON_BACK);
        msg_gamepad.btn_shoulder_l_state = get_button_state(BUTTON_LEFT_SHOULDER);
        msg_gamepad.btn_shoulder_r_state = get_button_state(BUTTON_RIGHT_SHOULDER);
        msg_gamepad.btn_xbox_state = get_button_state(BUTTON_XBOX);
        msg_gamepad.btn_thumb_l_state = get_button_state(BUTTON_LEFT_THUMB);
        msg_gamepad.btn_thumb_r_state = get_button_state(BUTTON_RIGHT_THUMB);
        msg_gamepad.btn_dpad_l_state = get_button_state(BUTTON_DPAD_LEFT);
        msg_gamepad.btn_dpad_r_state = get_button_state(BUTTON_DPAD_RIGHT );
        msg_gamepad.btn_dpad_u_state = get_button_state(BUTTON_DPAD_UP );
        msg_gamepad.btn_dpad_d_state = get_button_state(BUTTON_DPAD_DOWN );

        
        // Left trigger
        if (twist_lock && GamepadTriggerLength(controller, TRIGGER_LEFT) < 0.1)
            msg_gamepad.trg_l_val = 0.0;

        else
        {
            msg_gamepad.trg_l_val = GamepadTriggerLength(controller, TRIGGER_LEFT) - offset;
            msg_gamepad.trg_l_val = (msg_gamepad.trg_l_val > 0.0) ? msg_gamepad.trg_l_val/(1 - offset): msg_gamepad.trg_l_val/(offset);
          
            // Look for invalid input
            if ((msg_gamepad.trg_l_val < 0.01 && msg_gamepad.trg_l_val > -0.01) || isnan(msg_gamepad.trg_l_val))
                msg_gamepad.trg_l_val = 0.0;

            twist_lock = false;
        }

        // Right trigger
        if (hat_lock && GamepadTriggerLength(controller, TRIGGER_RIGHT) < 0.1)
            msg_gamepad.trg_r_val = 0.0;

        else
        {
            msg_gamepad.trg_r_val = GamepadTriggerLength(controller, TRIGGER_RIGHT)-offset;
            msg_gamepad.trg_r_val = (msg_gamepad.trg_r_val>0.0) ? msg_gamepad.trg_r_val/(1-offset): msg_gamepad.trg_r_val/(offset); //Re-scale OFFSET value
            
            // Look for invalid input
            if ((msg_gamepad.trg_r_val < 0.01 && msg_gamepad.trg_r_val > -0.01) || isnan(msg_gamepad.trg_r_val)) // Get rid of tiny floats
                msg_gamepad.trg_r_val = 0.0;
            
            hat_lock = false;
        }

        // When the joystick is first connected the twist and hat give a 0.0 until moved, whereas their actual centre is 0.435. This ensures that they have been moved first so they don't make a full negative power to twist and hat on connection
        msg_gamepad.btn_trigger_l_down = GamepadTriggerDown(controller, TRIGGER_LEFT);
        msg_gamepad.btn_trigger_r_down = GamepadTriggerDown(controller, TRIGGER_RIGHT);

        // Get DPAD inputs
        bool dpad_l = GamepadButtonDown(controller, BUTTON_DPAD_LEFT);
        bool dpad_r = GamepadButtonDown(controller, BUTTON_DPAD_RIGHT);
        bool dpad_u = GamepadButtonDown(controller, BUTTON_DPAD_UP);
        bool dpad_d = GamepadButtonDown(controller, BUTTON_DPAD_DOWN);

        // Calculate the DPAD data
        msg_gamepad.ax_dpad_x = dpad_r - dpad_l; // Left -1, none/both 0, right 1
        msg_gamepad.ax_dpad_y = dpad_u - dpad_d; // Down -1, none/both 0, up 1
        
    }

    // If the input is not connected, lock the inputs and reset the message
    else
    {
        msg_gamepad = core::msg::InputGamepad();
        twist_lock = true;
        hat_lock = true;
    }
}


/*
    Fetches rest of controller values and updates the message object
*/
void Joystick::set_message_values_joystick() {

    // Checks if the gamepad is currently connected
    msg_joystick.connected = GamepadIsConnected(controller);

    // If the gamepad is connected
    if (msg_joystick.connected)
    {          
        // Set the values in the ROS msg to the main stick
        msg_joystick.ax_stick_x = stick_lx_f;
        msg_joystick.ax_stick_y = stick_ly_f;
        msg_joystick.ax_stick_twist = convert_trg2ax(GamepadTriggerLength(controller, TRIGGER_LEFT));

        // Set the values in the ROS msg to thumb stick
        msg_joystick.ax_thumb_x = -stick_ry_f;
        msg_joystick.ax_thumb_y = -convert_trg2ax(GamepadTriggerLength(controller, TRIGGER_RIGHT));

        // Set the values of the slider
        msg_joystick.ax_slider = convert_ax2trg(-stick_rx_f);

        // Set the state messages of each of the buttons
        msg_joystick.btn_thumb_l_state = get_button_state(BUTTON_X);
        msg_joystick.btn_thumb_r_state = get_button_state(BUTTON_Y);
        msg_joystick.btn_thumb_u_state = get_button_state(BUTTON_A);
        msg_joystick.btn_thumb_d_state = get_button_state(BUTTON_B);

        msg_joystick.btn_bottom_l1_state = get_button_state(BUTTON_RIGHT_THUMB);
        msg_joystick.btn_bottom_l2_state = get_button_state(BUTTON_DPAD_UP);
        msg_joystick.btn_bottom_l3_state = 0;
        msg_joystick.btn_bottom_l4_state = 0;
        msg_joystick.btn_bottom_l5_state = 0;
        msg_joystick.btn_bottom_l6_state = 0;

        msg_joystick.btn_bottom_r1_state = get_button_state(BUTTON_BACK);
        msg_joystick.btn_bottom_r2_state = get_button_state(BUTTON_RIGHT_SHOULDER);
        msg_joystick.btn_bottom_r3_state = get_button_state(BUTTON_LEFT_SHOULDER);
        msg_joystick.btn_bottom_r4_state = get_button_state(BUTTON_START);
        msg_joystick.btn_bottom_r5_state = get_button_state(BUTTON_XBOX);
        msg_joystick.btn_bottom_r6_state = get_button_state(BUTTON_LEFT_THUMB);

        return;

        
        // Left trigger
        if (twist_lock && GamepadTriggerLength(controller, TRIGGER_LEFT) < 0.1)
            msg_gamepad.trg_l_val = 0.0;

        else
        {
            msg_gamepad.trg_l_val = GamepadTriggerLength(controller, TRIGGER_LEFT) - offset;
            msg_gamepad.trg_l_val = (msg_gamepad.trg_l_val > 0.0) ? msg_gamepad.trg_l_val/(1 - offset): msg_gamepad.trg_l_val/(offset);
          
            // Look for invalid input
            if ((msg_gamepad.trg_l_val < 0.01 && msg_gamepad.trg_l_val > -0.01) || isnan(msg_gamepad.trg_l_val))
                msg_gamepad.trg_l_val = 0.0;

            twist_lock = false;
        }

        // Right trigger
        if (hat_lock && GamepadTriggerLength(controller, TRIGGER_RIGHT) < 0.1)
            msg_gamepad.trg_r_val = 0.0;

        else
        {
            msg_gamepad.trg_r_val = GamepadTriggerLength(controller, TRIGGER_RIGHT)-offset;
            msg_gamepad.trg_r_val = (msg_gamepad.trg_r_val>0.0) ? msg_gamepad.trg_r_val/(1-offset): msg_gamepad.trg_r_val/(offset); //Re-scale OFFSET value
            
            // Look for invalid input
            if ((msg_gamepad.trg_r_val < 0.01 && msg_gamepad.trg_r_val > -0.01) || isnan(msg_gamepad.trg_r_val)) // Get rid of tiny floats
                msg_gamepad.trg_r_val = 0.0;
            
            hat_lock = false;
        }
    }

    // If the input is not connected, lock the inputs and reset the message
    else
    {
        msg_joystick = core::msg::InputJoystick();
        twist_lock = true;
        hat_lock = true;
    }
}


/*
    Returns the message object from the instance
*/
core::msg::InputGamepad Joystick::get_message_gamepad() {
    return msg_gamepad;
}

/*
    Returns the message object from the instance
*/
core::msg::InputJoystick Joystick::get_message_joystick() {
    return msg_joystick;
}


/*
    Returns the sign of the input float (-1 or 1).
*/
int Joystick::sign(const float val) {
    return (int)(0.0 < val) - (val < 0.0);
}

// (0 to 1) to (-1 to 1)
float Joystick::convert_trg2ax (const float val) {
    return val * 2.0 - 1.0;
}

// (-1 to 1) to (0 to 1)
float Joystick::convert_ax2trg (const float val) {
    return (val + 1.0) / 2.0;
}