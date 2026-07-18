/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Matthew Gu
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_inputs/keyboard_translate.h"
#include "inputs/keyboard.h"

#include <SDL2/SDL.h>
#include "colors.h"
#include <string>
#include <iostream>

KeyboardTranslate::KeyboardTranslate(): speed(0.5) {
    set_key_mappings();
}

void KeyboardTranslate::set_key_mappings(){
    // Control scheme
    key_mappings.input_lock = ctrl(SDL_SCANCODE_L);
    key_mappings.joint_limits = ctrl(SDL_SCANCODE_RETURN);
    key_mappings.ik_linear = ctrl(SDL_SCANCODE_5);
    key_mappings.ik_angular = ctrl(SDL_SCANCODE_6);
    key_mappings.flat_frame_linear = ctrl(SDL_SCANCODE_1);
    key_mappings.flat_frame_angular = ctrl(SDL_SCANCODE_2);
    key_mappings.endpoint_frame_linear = ctrl(SDL_SCANCODE_3);
    key_mappings.endpoint_frame_angular = ctrl(SDL_SCANCODE_4);
    key_mappings.use_spm_roll = ctrl(SDL_SCANCODE_7);
    key_mappings.position_control = ctrl(SDL_SCANCODE_8); // not implemented
    key_mappings.all_joint_space = ctrl(SDL_SCANCODE_SPACE);
    key_mappings.all_task_space = ctrl(SDL_SCANCODE_9); // not implemented
    key_mappings.toggle_input = ctrl(SDL_SCANCODE_0);
    
    key_mappings.zero_resolvers = alt(SDL_SCANCODE_J);
    key_mappings.resolver_to_key = {
        {1, alt(SDL_SCANCODE_1)}, // key for resolver J1, J2 etc
        {2, alt(SDL_SCANCODE_2)},
        {3, alt(SDL_SCANCODE_3)},
        {4, alt(SDL_SCANCODE_4)},
        {5, alt(SDL_SCANCODE_5)},
        {6, alt(SDL_SCANCODE_6)},
    };
    
    // Shift based
    // note that equals is really plus on the keyboard
    key_mappings.speed_increase = shift(SDL_SCANCODE_EQUALS);
    key_mappings.speed_decrease = shift(SDL_SCANCODE_MINUS);
    key_mappings.base_frame_offset_toggle = shift(SDL_SCANCODE_TAB);

    // End Effector
    key_mappings.end_effector_actuation_increase = alt(SDL_SCANCODE_RIGHT);
    key_mappings.end_effector_actuation_decrease = alt(SDL_SCANCODE_LEFT);
    key_mappings.linear_actuation_increase = alt(SDL_SCANCODE_UP);
    key_mappings.linear_actuation_decrease = alt(SDL_SCANCODE_DOWN);
    // key_mappings.laser = ctrl(SDL_SCANCODE_RETURN);
    // key_mappings.hex_key_increase = SDL_SCANCODE_;
    // key_mappings.hex_key_decrease = SDL_SCANCODE_;
    // key_mappings.finger_increase = SDL_SCANCODE_;
    // key_mappings.finger_decrease = SDL_SCANCODE_;

    // Joint space control
    key_mappings.joint_1_increase = SDL_SCANCODE_Q;
    key_mappings.joint_1_decrease = SDL_SCANCODE_E;
    key_mappings.joint_2_increase = SDL_SCANCODE_A;
    key_mappings.joint_2_decrease = SDL_SCANCODE_D;
    key_mappings.joint_3_increase = SDL_SCANCODE_Z;
    key_mappings.joint_3_decrease = SDL_SCANCODE_C;
    key_mappings.joint_4_increase = SDL_SCANCODE_I;
    key_mappings.joint_4_decrease = SDL_SCANCODE_P;
    key_mappings.joint_5_increase = SDL_SCANCODE_J;
    key_mappings.joint_5_decrease = SDL_SCANCODE_L;
    key_mappings.joint_6_increase = SDL_SCANCODE_N;
    key_mappings.joint_6_decrease = SDL_SCANCODE_COMMA;

    // Task space control
    key_mappings.x_increase = SDL_SCANCODE_W;
    key_mappings.x_decrease = SDL_SCANCODE_S;
    key_mappings.y_increase = SDL_SCANCODE_A;
    key_mappings.y_decrease = SDL_SCANCODE_D;
    key_mappings.z_increase = SDL_SCANCODE_Q;
    key_mappings.z_decrease = SDL_SCANCODE_E;
    key_mappings.roll_increase = SDL_SCANCODE_L;
    key_mappings.roll_decrease = SDL_SCANCODE_J;
    key_mappings.pitch_increase = SDL_SCANCODE_I;
    key_mappings.pitch_decrease = SDL_SCANCODE_K;
    key_mappings.yaw_increase = SDL_SCANCODE_O;
    key_mappings.yaw_decrease = SDL_SCANCODE_U;
}

bool KeyboardTranslate::get_control_scheme_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs) {
        // Used here for determining whether printing is needed
    if (is_pressed(key_mappings.base_frame_offset_toggle) && !updated_controls) {
        updated_controls = true;
        // cycles through -1, 0, 1, indicating
        base_frame_offset++;
        if (base_frame_offset >= 2) {
            base_frame_offset = -1;
        }
        message = "Base frame offset: " + std::to_string(90*base_frame_offset);
        std::cout <<  message << "\n"; 
    }
    control_scheme_inputs.base_frame_offset = base_frame_offset;
    // turn off zeroing as it should be a temporary signal
    control_scheme_inputs.zero_resolvers = 0;
    // Zeroing the resolver
    if (is_pressed_or_held(key_mappings.zero_resolvers) && !updated_controls){
        // zero each of the 6 resolvers with 1~6
        for (int i = 1; i <= 6; i++){
            if (is_pressed_or_held(key_mappings.resolver_to_key[i])){
                control_scheme_inputs.zero_resolvers = i;
                updated_controls = true;
                message = "Zeroing resolver: " + std::to_string(i);
                std::cout << message << "\n";
            }
        }
    }
    
    // Arm lock
    toggle_control("Keyboard lock", control_scheme_inputs.input_lock, key_mappings.input_lock);
    // Joint limits
    toggle_control("Joint Limits", control_scheme_inputs.joint_limits, key_mappings.joint_limits);

    // Control schemes
    // Flat frame control
    toggle_control("Flat frame linear", control_scheme_inputs.flat_frame_linear, key_mappings.flat_frame_linear);
    toggle_control("Flat frame angular", control_scheme_inputs.flat_frame_angular, key_mappings.flat_frame_angular);

    // Endpoint frame control. Hold trigger
    // Also set if flat frame control is used
    toggle_control("Endpoint frame linear", control_scheme_inputs.endpoint_frame_linear, key_mappings.endpoint_frame_linear);
    control_scheme_inputs.endpoint_frame_linear = control_scheme_inputs.endpoint_frame_linear || control_scheme_inputs.flat_frame_linear;
    toggle_control("Endpoint frame angular", control_scheme_inputs.endpoint_frame_angular, key_mappings.endpoint_frame_angular);
    control_scheme_inputs.endpoint_frame_angular = control_scheme_inputs.endpoint_frame_angular || control_scheme_inputs.flat_frame_angular;

    // IK. Hold inside thumb button.
    // Also set if endpoint frame control is used.
    toggle_control("IK linear", control_scheme_inputs.ik_linear, key_mappings.ik_linear);
    control_scheme_inputs.ik_linear = control_scheme_inputs.ik_linear || control_scheme_inputs.endpoint_frame_linear;
    toggle_control("IK angular", control_scheme_inputs.ik_angular, key_mappings.ik_angular);
    control_scheme_inputs.ik_angular = control_scheme_inputs.ik_angular || control_scheme_inputs.endpoint_frame_angular;

    // Set SPM roll handling. Hold back thumb button on right stick
    toggle_control("Use SPM roll", control_scheme_inputs.use_spm_roll, key_mappings.use_spm_roll);

    // All Joint space
    if (is_pressed(key_mappings.all_joint_space) && !updated_controls) {
        updated_controls = true;
        control_scheme_inputs.ik_linear = false;
        control_scheme_inputs.ik_angular = false;
        control_scheme_inputs.flat_frame_linear = false;
        control_scheme_inputs.flat_frame_angular = false;
        control_scheme_inputs.endpoint_frame_linear = false;
        control_scheme_inputs.endpoint_frame_angular = false;
        std::cout << "Joint Space: On" << "\n";
    }

    // Correction for position control - can't have independent linear and angular control
    // Not yet implemented
    if (control_scheme_inputs.position_control && !updated_controls) {
        control_scheme_inputs.flat_frame_angular = control_scheme_inputs.flat_frame_linear;
        control_scheme_inputs.endpoint_frame_angular = control_scheme_inputs.endpoint_frame_linear;
        control_scheme_inputs.ik_angular = control_scheme_inputs.ik_linear;
    }
    return is_pressed(key_mappings.toggle_input);
}

void KeyboardTranslate::get_end_effector_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs, arm_interfaces::msg::EndEffectorInput& end_effector_inputs) {
    change_speed();
    if (!control_scheme_inputs.input_lock){
        // Set the values for linear actuator and end effector actuation
        end_effector_inputs.linear_actuation = (is_pressed_or_held(key_mappings.linear_actuation_increase)-is_pressed_or_held(key_mappings.linear_actuation_decrease));
        end_effector_inputs.end_effector_actuation = (is_pressed_or_held(key_mappings.end_effector_actuation_increase)-is_pressed_or_held(key_mappings.end_effector_actuation_decrease));
    } else {
        end_effector_inputs.linear_actuation = 0;
        end_effector_inputs.end_effector_actuation = 0;
    }
}

void KeyboardTranslate::get_joint_velocity_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs, sensor_msgs::msg::JointState& joint_velocity_inputs) {
    change_speed();
    joint_velocity_inputs.velocity.clear();
    if (!control_scheme_inputs.input_lock && !control_scheme_inputs.ik_linear) {
        // No speed scaling for lower joints;
        joint_velocity_inputs.velocity.push_back(speed * (is_pressed_or_held(key_mappings.joint_1_increase)-is_pressed_or_held(key_mappings.joint_1_decrease)));
        joint_velocity_inputs.velocity.push_back(speed * (is_pressed_or_held(key_mappings.joint_2_increase)-is_pressed_or_held(key_mappings.joint_2_decrease)));
        joint_velocity_inputs.velocity.push_back(speed * (is_pressed_or_held(key_mappings.joint_3_increase)-is_pressed_or_held(key_mappings.joint_3_decrease)));
    
    }
    else{
        joint_velocity_inputs.velocity.push_back(0);
        joint_velocity_inputs.velocity.push_back(0);
        joint_velocity_inputs.velocity.push_back(0);
    }

    // If using wrist joint-space control
    if (!control_scheme_inputs.input_lock && !control_scheme_inputs.ik_angular) {
        // Scale speed for wrist joints
        float speed_wrist_joints = speed * speed_multipliers.wrist_joints;
        
        joint_velocity_inputs.velocity.push_back(speed_wrist_joints * (is_pressed_or_held(key_mappings.joint_4_increase)-is_pressed_or_held(key_mappings.joint_4_decrease)));
        joint_velocity_inputs.velocity.push_back(speed_wrist_joints * (is_pressed_or_held(key_mappings.joint_5_increase)-is_pressed_or_held(key_mappings.joint_5_decrease)));
        joint_velocity_inputs.velocity.push_back(speed_wrist_joints * (is_pressed_or_held(key_mappings.joint_6_increase)-is_pressed_or_held(key_mappings.joint_6_decrease)));
    }
    else{
        joint_velocity_inputs.velocity.push_back(0);
        joint_velocity_inputs.velocity.push_back(0);
        joint_velocity_inputs.velocity.push_back(0);
    }

}

void KeyboardTranslate::get_twist_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs, geometry_msgs::msg::TwistStamped& twist_inputs) {
    change_speed();
    // If using lower joints IK, set the values for linear velocity
    if (!control_scheme_inputs.input_lock && control_scheme_inputs.ik_linear) {
        // Scale speed for linear IK
        float speed_ik_linear = speed * speed_multipliers.ik_linear;

        // Linear velocities map directly
        twist_inputs.twist.linear.x = speed_ik_linear * (is_pressed_or_held(key_mappings.x_increase)-is_pressed_or_held(key_mappings.x_decrease));
        twist_inputs.twist.linear.y = speed_ik_linear * (is_pressed_or_held(key_mappings.y_increase)-is_pressed_or_held(key_mappings.y_decrease));
        twist_inputs.twist.linear.z = speed_ik_linear * (is_pressed_or_held(key_mappings.z_increase)-is_pressed_or_held(key_mappings.z_decrease));
    }
    else {
        twist_inputs.twist.linear.x = 0;
        twist_inputs.twist.linear.y = 0;
        twist_inputs.twist.linear.z = 0;
    }
    // If using wrist IK, set the values for angular velocity
    if (!control_scheme_inputs.input_lock && control_scheme_inputs.ik_angular) {
        // Scale speed for angular IK
        float speed_ik_angular = speed * speed_multipliers.ik_angular;
        
        // Adjust roll and pitch directions so control is more intuitive
        // Equivalent to a rotation of the input angular velocity vector by +pi/2 about z axis
        // roll pitch yaw
        twist_inputs.twist.angular.x = speed_ik_angular * (is_pressed_or_held(key_mappings.roll_increase)-is_pressed_or_held(key_mappings.roll_decrease));
        twist_inputs.twist.angular.y = speed_ik_angular * (is_pressed_or_held(key_mappings.pitch_increase)-is_pressed_or_held(key_mappings.pitch_decrease));
        twist_inputs.twist.angular.z = speed_ik_angular * (is_pressed_or_held(key_mappings.yaw_increase)-is_pressed_or_held(key_mappings.yaw_decrease));
    }
    else{
        twist_inputs.twist.angular.x = 0;
        twist_inputs.twist.angular.y = 0;
        twist_inputs.twist.angular.z = 0;
    }
}

void KeyboardTranslate::keyboard_callback(input_interfaces::msg::InputKeyboard::SharedPtr msg) {
    keyboard = *msg;
    read_keys();
}

void KeyboardTranslate::reset_message()
{
    keyboard = input_interfaces::msg::InputKeyboard();
    read_keys();

}

bool KeyboardTranslate::is_connected()
{
    return keyboard.connected;
}

void KeyboardTranslate::read_keys()
{
    updated_controls = false;
    updated_speed = false;
    key_pressed_set.clear();
    for (auto key : keyboard.keys_pressed){
        key_pressed_set.insert(key);
    }
    key_held_set.clear();
    for (auto key : keyboard.keys_repeated){
        key_held_set.insert(key);
    }
}

bool KeyboardTranslate::is_pressed(uint32_t key)
{
    return key_pressed_set.count(key);
}

bool KeyboardTranslate::is_held(uint32_t key)
{
    return key_held_set.count(key);
}

bool KeyboardTranslate::is_pressed_or_held(uint32_t key)
{
    return is_pressed(key) || is_held(key);
}

void KeyboardTranslate::toggle_control(std::string field_name, bool& value, uint32_t key)
{   
    if (is_pressed(key) && !updated_controls){
        value = !value;
        updated_controls = true;
        message = field_name + (value?": true":": false");
        std::cout << C_MODE << message.c_str() << C_END << "\n";
    }
}

uint32_t KeyboardTranslate::ctrl(uint32_t key)
{
    return key | Keyboard::CTRL_MASK;
}

uint32_t KeyboardTranslate::shift(uint32_t key)
{
    return key | Keyboard::SHIFT_MASK;
}

uint32_t KeyboardTranslate::alt(uint32_t key)
{
    return key | Keyboard::ALT_MASK;
}

inline void KeyboardTranslate::change_speed(){
    if (!updated_speed){
        updated_speed = true;
        if (is_pressed(key_mappings.speed_increase)) {
            display_speed = std::min(display_speed + speed_increment, 1.0f);
            message = "Speed: " + std::to_string(display_speed);
            std::cout << message.c_str() << "\n";
        } else if (is_pressed(key_mappings.speed_decrease)) {
            display_speed = std::max(display_speed - speed_increment, 0.0f);
            message = "Speed: " + std::to_string(display_speed);
            std::cout << message.c_str() << "\n";
        }
        speed = display_speed * speed_multipliers.all_inputs;
    }
}
