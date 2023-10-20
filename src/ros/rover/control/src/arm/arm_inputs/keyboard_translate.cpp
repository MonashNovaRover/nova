/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jess Hepworth, Jory Braun, Matthew Gu
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "keyboard_translate.h"
#include <linux/input.h>
#include "print/print.h"

KeyboardTranslate::KeyboardTranslate(): joint_twist_speed(0), end_effector_speed(0) {}

CommonInputCollections::ControlSchemeInputs KeyboardTranslate::get_arm_lock_inputs() {
    control_scheme_inputs.control_scheme_update = false;
    // Arm lock
    if (is_ctrl() && is_pressed(KEY_L)) {
        if (!control_scheme_inputs.input_lock) {
            Print::print("Keyboard locked");
            control_scheme_inputs.input_lock = true;
        }
        else{
            Print::print("Keyboard Unlocked");
            control_scheme_inputs.input_lock = false;
        }
        control_scheme_inputs.control_scheme_update = true;
    }
    // Joint limits
    if (is_ctrl() && is_pressed(KEY_ENTER)){
        control_scheme_inputs.joint_limits = !control_scheme_inputs.joint_limits;
        control_scheme_inputs.control_scheme_update = true;
    }
    // TODO: Position control
    return control_scheme_inputs;
}

CommonInputCollections::ControlSchemeInputs KeyboardTranslate::get_control_scheme_inputs() {
    // Set base reference frame offset
    int8_t base_frame_offset = 0;
    // seems to only allow left or right? Not too sure. 
    if (is_ctrl() && is_pressed(KEY_BACKSPACE)) {
        base_frame_offset = -1;
    }
    else if (is_ctrl() && is_pressed(KEY_DELETE)) {
        base_frame_offset = 1;
    }
    control_scheme_inputs.base_frame_offset = base_frame_offset;

    // Question: Can I enable multiple control schemes at the same time?

    // Control schemes
    // Flat frame control
    control_scheme_inputs.flat_frame_linear = is_ctrl() && is_held(KEY_1);
    control_scheme_inputs.flat_frame_angular = is_ctrl() && is_held(KEY_2);
    // Endpoint frame control. Hold trigger
    // Also set if flat frame control is used
    control_scheme_inputs.endpoint_frame_linear = is_ctrl() && is_held(KEY_3) || control_scheme_inputs.flat_frame_linear;
    control_scheme_inputs.endpoint_frame_angular = is_ctrl() && is_held(KEY_4) || control_scheme_inputs.flat_frame_angular;
    // IK. Hold inside thumb button.
    // Also set if endpoint frame control is used.
    control_scheme_inputs.ik_linear = is_ctrl() && is_held(KEY_5) || control_scheme_inputs.endpoint_frame_linear;
    control_scheme_inputs.ik_angular = is_ctrl() && is_held(KEY_6) || control_scheme_inputs.endpoint_frame_angular;
    // Set SPM roll handling. Hold back thumb button on right stick
    control_scheme_inputs.use_spm_roll = is_ctrl() && is_held(KEY_7);

    // Correction for position control - can't have independent linear and angular control
    if (control_scheme_inputs.position_control) {
        control_scheme_inputs.flat_frame_angular = control_scheme_inputs.flat_frame_linear;
        control_scheme_inputs.endpoint_frame_angular = control_scheme_inputs.endpoint_frame_linear;
        control_scheme_inputs.ik_angular = control_scheme_inputs.ik_linear;
    }
    return control_scheme_inputs;

}

CommonInputCollections::EndEffectorInputs KeyboardTranslate::get_end_effector_inputs() {
    if (is_shift()) {
        if (is_pressed(KEY_LEFT)) {
            end_effector_speed = increase_speed(end_effector_speed);
        } else if (is_pressed(KEY_RIGHT)) {
            end_effector_speed = decrease_speed(end_effector_speed);
        }
    }

    if (!control_scheme_inputs.input_lock){
        // Set the values for linear actuator and end effector actuation
        end_effector_inputs.linear_actuation = is_pressed_or_held(KEY_W);
        end_effector_inputs.end_effector_actuation = is_pressed_or_held(KEY_O) * 0.95;
    }
    return end_effector_inputs;
}

CommonInputCollections::JointVelocityInputs KeyboardTranslate::get_joint_velocity_inputs() {
    if (is_shift()) {
        if (is_pressed(KEY_UP)) {
            joint_twist_speed = increase_speed(joint_twist_speed);
        } else if (is_pressed(KEY_DOWN)) {
            joint_twist_speed = decrease_speed(joint_twist_speed);
        }
    }
    float speed = joint_twist_speed * speed_multipliers.all_inputs;
    if (!control_scheme_inputs.input_lock && !control_scheme_inputs.ik_linear) {
        // No speed scaling for lower joints;
        
        // Base rotation is stick twist. CCW rotates arm CCW (from above)
        joint_velocity_inputs.velocities[0] = speed * (is_pressed_or_held(KEY_Q)-is_pressed_or_held(KEY_E));
        // Shoulder is stick y (left-right). Left moves the arm towards the back of the rover
        joint_velocity_inputs.velocities[1] = speed * (is_pressed_or_held(KEY_A)-is_pressed_or_held(KEY_D));
        // Elbow is stick x (forward-backward). Forward pitches arm down
        joint_velocity_inputs.velocities[2] = speed * (is_pressed_or_held(KEY_Z)-is_pressed_or_held(KEY_C));
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
        joint_velocity_inputs.velocities[3] = speed_wrist_joints * (is_pressed_or_held(KEY_I)-is_pressed_or_held(KEY_P));
        // J5 is stick y. Left yaws arm left
        joint_velocity_inputs.velocities[4] = speed_wrist_joints * (is_pressed_or_held(KEY_J)-is_pressed_or_held(KEY_L));
        // J6 is stick twist. CCW tilts end effector CCW (looking out from end effector)
        joint_velocity_inputs.velocities[5] = speed_wrist_joints * (is_pressed_or_held(KEY_N)-is_pressed_or_held(KEY_COMMA));
    }
    else{
        joint_velocity_inputs.velocities[3] = 0;
        joint_velocity_inputs.velocities[4] = 0;
        joint_velocity_inputs.velocities[5] = 0;
    }

    return joint_velocity_inputs;


}

CommonInputCollections::TwistInputs KeyboardTranslate::get_twist_inputs() {
    if (is_shift()) {
        if (is_pressed(KEY_UP)) {
            joint_twist_speed = increase_speed(joint_twist_speed);
        } else if (is_pressed(KEY_DOWN)) {
            joint_twist_speed = decrease_speed(joint_twist_speed);
        }
    }
    float speed = joint_twist_speed * speed_multipliers.all_inputs;
    
    // If using lower joints IK, set the values for linear velocity
    if (!control_scheme_inputs.input_lock && control_scheme_inputs.ik_linear) {
        // Scale speed for linear IK
        float speed_ik_linear = speed * speed_multipliers.ik_linear;

        // Linear velocities map directly from joystick. Directions are already in arm base coords
        twist_inputs.linear.x = speed_ik_linear * (is_pressed_or_held(KEY_Q)-is_pressed_or_held(KEY_E));
        twist_inputs.linear.y = speed_ik_linear * (is_pressed_or_held(KEY_A)-is_pressed_or_held(KEY_D));
        twist_inputs.linear.z = speed_ik_linear * (is_pressed_or_held(KEY_Z)-is_pressed_or_held(KEY_C));
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
        twist_inputs.angular.x = speed_ik_angular * (is_pressed_or_held(KEY_I)-is_pressed_or_held(KEY_P));
        // Pitch is stick x (forward-backward)
        twist_inputs.angular.y = speed_ik_angular * (is_pressed_or_held(KEY_J)-is_pressed_or_held(KEY_L));
        // Yaw is stick twist
        twist_inputs.angular.z = speed_ik_angular * (is_pressed_or_held(KEY_N)-is_pressed_or_held(KEY_COMMA));
    }
    else{
        twist_inputs.angular.x = 0;
        twist_inputs.angular.y = 0;
        twist_inputs.angular.z = 0;
    }
    return twist_inputs;
}

void KeyboardTranslate::set_message(std::shared_ptr<void> msg, int idx) {
    // static_point_cast may be faster here
    auto keyboardMessage = std::dynamic_pointer_cast<core::msg::InputKeyboard>(msg);
    if (!keyboardMessage || idx != 0) {
        Print::print("Invalid message type for keyboard translate");
        return;
    } else {
        keyboard = *keyboardMessage;
    }
}

void KeyboardTranslate::reset_message()
{
    keyboard = core::msg::InputKeyboard();
}

bool KeyboardTranslate::is_connected()
{
    return keyboard.is_connected;
}

bool KeyboardTranslate::is_released(int key)
{
    for (long unsigned int i = 0; i < keyboard.keys_released.size(); i++) {
        if (key == keyboard.keys_released[i]) {
            return true
        }
    }
    return false;
}

bool KeyboardTranslate::is_pressed(int key)
{
    for (long unsigned int i = 0; i < keyboard.keys_pressed.size(); i++) {
        if (key == keyboard.keys_pressed[i]) {
            return true
        }
    }
    return false;
}

bool KeyboardTranslate::is_held(int key)
{
    for (long unsigned int i = 0; i < keyboard.keys_repeated.size(); i++) {
        if (key == keyboard.keys_repeated[i]) {
            return true
        }
    }
    return false;
}

bool KeyboardTranslate::is_pressed_or_held(int key)
{
    return is_pressed(key) || is_held(key);
}

bool KeyboardTranslate::is_ctrl()
{
    return is_pressed_or_held(KEY_LEFTCTRL) || is_pressed_or_held(KEY_RIGHTCTRL);
}

bool KeyboardTranslate::is_shift()
{
    return is_pressed_or_held(KEY_LEFTSHIFT) || is_pressed_or_held(KEY_RIGHTSHIFT);
}

float KeyboardTranslate::increase_speed(float value)
{
    return std::min(value + speed_increment, 1.0f);
}

float KeyboardTranslate::decrease_speed(float value)
{
    return std::max(value - speed_increment, 0.0f);
}
