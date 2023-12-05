/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Matthew Gu
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "keyboard_translate.h"
#include <SDL2/SDL.h>
#include "print/print.h"

KeyboardTranslate::KeyboardTranslate(): joint_twist_speed(0), end_effector_speed(0) {}

CommonInputCollections::ControlSchemeInputs KeyboardTranslate::get_arm_lock_inputs() {
    control_scheme_inputs.control_scheme_update = false;
    // Arm lock
    toggle_control("Input lock", control_scheme_inputs.arm_lock, ctrl(SDL_SCANCODE_L));
    // Joint limits
    toggle_control("Joint Limits", control_scheme_inputs.joint_limits, ctrl(SDL_SCANCODE_RETURN));

    return control_scheme_inputs;
}

CommonInputCollections::ControlSchemeInputs KeyboardTranslate::get_control_scheme_inputs() {
    // Used here for determining whether printing is needed
    if (is_pressed(ctrl(SDL_SCANCODE_TAB))) {
        base_frame_offset++;
        if (base_frame_offset >= 2) {
            base_frame_offset = -1;
        }
        message = "Base frame offset: " + std::to_string(base_frame_offset);
        Print::print(message.c_str());
    }
    control_scheme_inputs.base_frame_offset = base_frame_offset;
    // Control schemes
    // Flat frame control
    toggle_control("Flat frame linear", control_scheme_inputs.flat_frame_linear, ctrl(SDL_SCANCODE_1));
    toggle_control("Flat frame angular", control_scheme_inputs.flat_frame_angular, ctrl(SDL_SCANCODE_2));

    // Endpoint frame control. Hold trigger
    // Also set if flat frame control is used
    toggle_control("Endpoint frame linear", control_scheme_inputs.endpoint_frame_linear, ctrl(SDL_SCANCODE_3));
    control_scheme_inputs.endpoint_frame_linear = control_scheme_inputs.endpoint_frame_linear || control_scheme_inputs.flat_frame_linear;
    toggle_control("Endpoint frame angular", control_scheme_inputs.endpoint_frame_angular, ctrl(SDL_SCANCODE_4));
    control_scheme_inputs.endpoint_frame_angular = control_scheme_inputs.endpoint_frame_angular || control_scheme_inputs.flat_frame_angular;

    // IK. Hold inside thumb button.
    // Also set if endpoint frame control is used.
    toggle_control("IK linear", control_scheme_inputs.ik_linear, ctrl(SDL_SCANCODE_5));
    control_scheme_inputs.ik_linear = control_scheme_inputs.ik_linear || control_scheme_inputs.endpoint_frame_linear;
    toggle_control("IK angular", control_scheme_inputs.ik_angular, ctrl(SDL_SCANCODE_6));
    control_scheme_inputs.ik_angular = control_scheme_inputs.ik_angular || control_scheme_inputs.endpoint_frame_angular;

    // Set SPM roll handling. Hold back thumb button on right stick
    toggle_control("Use SPM roll", control_scheme_inputs.use_spm_roll, ctrl(SDL_SCANCODE_7));

    // Joint space if 0
    if (is_pressed(ctrl(SDL_SCANCODE_0))) {
        control_scheme_inputs.ik_linear = false;
        control_scheme_inputs.ik_angular = false;
        control_scheme_inputs.flat_frame_linear = false;
        control_scheme_inputs.flat_frame_angular = false;
        control_scheme_inputs.endpoint_frame_linear = false;
        control_scheme_inputs.endpoint_frame_angular = false;
        control_scheme_inputs.control_scheme_update = true;
        Print::print("Joint Space: On");
    }

    // Correction for position control - can't have independent linear and angular control
    // Not yet implemented
    if (control_scheme_inputs.position_control) {
        control_scheme_inputs.flat_frame_angular = control_scheme_inputs.flat_frame_linear;
        control_scheme_inputs.endpoint_frame_angular = control_scheme_inputs.endpoint_frame_linear;
        control_scheme_inputs.ik_angular = control_scheme_inputs.ik_linear;
    }
    return control_scheme_inputs;

}

CommonInputCollections::EndEffectorInputs KeyboardTranslate::get_end_effector_inputs() {
    if (is_pressed(shift(SDL_SCANCODE_LEFT))) {
        increase_speed("EE Speed", end_effector_speed);
    } else if (is_pressed(shift(SDL_SCANCODE_RIGHT))) {
        decrease_speed("EE Speed", end_effector_speed);
    }

    if (!control_scheme_inputs.input_lock){
        // Set the values for linear actuator and end effector actuation
        end_effector_inputs.linear_actuation = is_pressed_or_held(SDL_SCANCODE_W);
        end_effector_inputs.end_effector_actuation = is_pressed_or_held(SDL_SCANCODE_O) * 0.95;
    }
    return end_effector_inputs;
}

CommonInputCollections::JointVelocityInputs KeyboardTranslate::get_joint_velocity_inputs() {
    if (is_pressed(shift(SDL_SCANCODE_UP))) {
        increase_speed("Joint Velocity", joint_twist_speed);
    } else if (is_pressed(shift(SDL_SCANCODE_DOWN))) {
        decrease_speed("Joint Velocity", joint_twist_speed);
    }

    float speed = joint_twist_speed * speed_multipliers.all_inputs;
    if (!control_scheme_inputs.input_lock && !control_scheme_inputs.ik_linear) {
        // No speed scaling for lower joints;
        
        // Base rotation is stick twist. CCW rotates arm CCW (from above)
        joint_velocity_inputs.velocities[0] = speed * (is_pressed_or_held(SDL_SCANCODE_Q)-is_pressed_or_held(SDL_SCANCODE_E));
        // Shoulder is stick y (left-right). Left moves the arm towards the back of the rover
        joint_velocity_inputs.velocities[1] = speed * (is_pressed_or_held(SDL_SCANCODE_A)-is_pressed_or_held(SDL_SCANCODE_D));
        // Elbow is stick x (forward-backward). Forward pitches arm down
        joint_velocity_inputs.velocities[2] = speed * (is_pressed_or_held(SDL_SCANCODE_Z)-is_pressed_or_held(SDL_SCANCODE_C));
    }
    else{
        joint_velocity_inputs.velocities[0] = 0;
        joint_velocity_inputs.velocities[1] = 0;
        joint_velocity_inputs.velocities[2] = 0;
    }

    // If using wrist joint-space control
    if (!control_scheme_inputs.input_lock && !control_scheme_inputs.ik_angular) {
        // Scale speed for wrist joints
        float speed_wrist_joints = joint_twist_speed * speed_multipliers.wrist_joints;
        
        // J4 is stick x. Forward pitches arm down
        joint_velocity_inputs.velocities[3] = speed_wrist_joints * (is_pressed_or_held(SDL_SCANCODE_I)-is_pressed_or_held(SDL_SCANCODE_P));
        // J5 is stick y. Left yaws arm left
        joint_velocity_inputs.velocities[4] = speed_wrist_joints * (is_pressed_or_held(SDL_SCANCODE_J)-is_pressed_or_held(SDL_SCANCODE_L));
        // J6 is stick twist. CCW tilts end effector CCW (looking out from end effector)
        joint_velocity_inputs.velocities[5] = speed_wrist_joints * (is_pressed_or_held(SDL_SCANCODE_N)-is_pressed_or_held(SDL_SCANCODE_COMMA));
    }
    else{
        joint_velocity_inputs.velocities[3] = 0;
        joint_velocity_inputs.velocities[4] = 0;
        joint_velocity_inputs.velocities[5] = 0;
    }

    return joint_velocity_inputs;


}

CommonInputCollections::TwistInputs KeyboardTranslate::get_twist_inputs() {
    if (is_pressed(shift(SDL_SCANCODE_UP))) {
        increase_speed("Joint Speed", joint_twist_speed);
    } else if (is_pressed(shift(SDL_SCANCODE_DOWN))) {
        decrease_speed("Joint Speed", joint_twist_speed);
    }
    float speed = joint_twist_speed * speed_multipliers.all_inputs;
    
    // If using lower joints IK, set the values for linear velocity
    if (!control_scheme_inputs.input_lock && control_scheme_inputs.ik_linear) {
        // Scale speed for linear IK
        float speed_ik_linear = speed * speed_multipliers.ik_linear;

        // Linear velocities map directly from joystick. Directions are already in arm base coords
        twist_inputs.linear.x = speed_ik_linear * (is_pressed_or_held(SDL_SCANCODE_Q)-is_pressed_or_held(SDL_SCANCODE_E));
        twist_inputs.linear.y = speed_ik_linear * (is_pressed_or_held(SDL_SCANCODE_A)-is_pressed_or_held(SDL_SCANCODE_D));
        twist_inputs.linear.z = speed_ik_linear * (is_pressed_or_held(SDL_SCANCODE_Z)-is_pressed_or_held(SDL_SCANCODE_C));
    }
    else {
        twist_inputs.linear.x = 0;
        twist_inputs.linear.y = 0;
        twist_inputs.linear.z = 0;
    }
    // If using wrist IK, set the values for angular velocity
    if (!control_scheme_inputs.input_lock && control_scheme_inputs.ik_angular) {
        // Scale speed for angular IK
        float speed_ik_angular = speed * speed_multipliers.ik_angular;
        
        // Adjust roll and pitch directions so control is more intuitive
        // Equivalent to a rotation of the input angular velocity vector by +pi/2 about z axis
        // Roll is stick y (left-right)
        twist_inputs.angular.x = speed_ik_angular * (is_pressed_or_held(SDL_SCANCODE_I)-is_pressed_or_held(SDL_SCANCODE_P));
        // Pitch is stick x (forward-backward)
        twist_inputs.angular.y = speed_ik_angular * (is_pressed_or_held(SDL_SCANCODE_J)-is_pressed_or_held(SDL_SCANCODE_L));
        // Yaw is stick twist
        twist_inputs.angular.z = speed_ik_angular * (is_pressed_or_held(SDL_SCANCODE_N)-is_pressed_or_held(SDL_SCANCODE_COMMA));
    }
    else{
        twist_inputs.angular.x = 0;
        twist_inputs.angular.y = 0;
        twist_inputs.angular.z = 0;
    }
    return twist_inputs;
}

void KeyboardTranslate::keyboard_callback(core::msg::InputKeyboard::SharedPtr msg) {
    keyboard = *msg;
}

void KeyboardTranslate::reset_message()
{
    keyboard = core::msg::InputKeyboard();
}

bool KeyboardTranslate::is_connected()
{
    return keyboard.connected;
}

bool KeyboardTranslate::is_pressed(uint32_t key)
{
    for (long unsigned int i = 0; i < keyboard.keys_pressed.size(); i++) {
        if (key == keyboard.keys_pressed[i]) {
            return true;
        }
    }
    return false;
}

bool KeyboardTranslate::is_held(uint32_t key)
{
    for (long unsigned int i = 0; i < keyboard.keys_repeated.size(); i++) {
        if (key == keyboard.keys_repeated[i]) {
            return true;
        }
    }
    return false;
}

bool KeyboardTranslate::is_pressed_or_held(uint32_t key)
{
    return is_pressed(key) || is_held(key);
}

void KeyboardTranslate::toggle_control(std::string field_name, bool& value, uint32_t key)
{   
    if (is_pressed(key)){
        value = !value;
        control_scheme_inputs.control_scheme_update = true;
        message = field_name + ": " + std::to_string(value?"true":"false");
        Print::print(message.c_str());
    }
}

uint32_t KeyboardTranslate::ctrl(uint32_t key)
{
    return key | CTRL_MASK;
}

uint32_t KeyboardTranslate::shift(uint32_t key)
{
    return key | SHIFT_MASK;
}

uint32_t KeyboardTranslate::alt(uint32_t key)
{
    return key | ALT_MASK;
}

void KeyboardTranslate::increase_speed(std::string field_name, float& value)
{
    value = std::min(value + speed_increment, 1.0f);
    message = field_name + ": " + std::to_string(value);
    Print::print(message.c_str());
}

void KeyboardTranslate::decrease_speed(std::string field_name, float& value)
{
    value = std::max(value - speed_increment, 0.0f);
    message = field_name + ": " + std::to_string(value);
    Print::print(message.c_str());
}
