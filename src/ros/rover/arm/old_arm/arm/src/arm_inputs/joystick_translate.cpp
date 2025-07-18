/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jess Hepworth, Jory Braun, Matthew Gu
LAST EDIT: Rohit Pilakkat
DATE: 13/4/25
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_inputs/joystick_translate.h"
#include "colors.h"
#include <iostream>

JoystickTranslate::JoystickTranslate() { }


bool JoystickTranslate::get_control_scheme_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs)
{
    // Set base reference frame offset
    int8_t base_frame_offset = 0;
    if (joystick_l.ax_slider < 0.3) {
        base_frame_offset = -1;
    }
    else if (joystick_l.ax_slider > 0.8) {
        base_frame_offset = 1;
    }
    control_scheme_inputs.base_frame_offset = base_frame_offset;
    
    // Arm lock
    if (joystick_l.btn_bottom_l2_state == 1) {
        if (!control_scheme_inputs.input_lock)
            std::cout << C_MODE << "Joysticks locked" << C_END << "\n";
        control_scheme_inputs.input_lock = true;
    }
    if (joystick_l.btn_bottom_l5_state == 1){
        if (control_scheme_inputs.input_lock)
            std::cout << "Joysticks Unlocked" << C_END << "\n";
        control_scheme_inputs.input_lock = false;
    }
    // Joint limits
    if (joystick_l.btn_bottom_l1_state == 1) {
        control_scheme_inputs.joint_limits = true;
    }
    if (joystick_l.btn_bottom_l4_state == 1) {
        control_scheme_inputs.joint_limits = false;
    }
#if POSITION_CONTROL_ENABLE
    // Position control
    if (joystick_l.btn_bottom_l3_state == 1) {
        control_scheme_inputs.position_control = true;
    }
    if (joystick_l.btn_bottom_l6_state == 1) {
        control_scheme_inputs.position_control = false;
    }
#endif

    // Control schemes
    // Flat frame control
    control_scheme_inputs.flat_frame_linear = joystick_l.btn_thumb_l_state == 2;
    control_scheme_inputs.flat_frame_angular = joystick_r.btn_thumb_r_state == 2;
    // Endpoint frame control. Hold trigger
    // Also set if flat frame control is used
    control_scheme_inputs.endpoint_frame_linear = joystick_l.btn_thumb_u_state == 2 || control_scheme_inputs.flat_frame_linear;
    control_scheme_inputs.endpoint_frame_angular = joystick_r.btn_thumb_u_state == 2 || control_scheme_inputs.flat_frame_angular;
    // IK. Hold inside thumb button.
    // Also set if endpoint frame control is used.
    control_scheme_inputs.ik_linear = joystick_l.btn_thumb_r_state == 2 || control_scheme_inputs.endpoint_frame_linear;
    control_scheme_inputs.ik_angular = joystick_r.btn_thumb_l_state == 2 || control_scheme_inputs.endpoint_frame_angular;
    // Set SPM roll handling. Hold back thumb button on right stick
    control_scheme_inputs.use_spm_roll = joystick_r.btn_thumb_d_state == 2;

    // Correction for position control - can't have independent linear and angular control
    if (control_scheme_inputs.position_control) {
        control_scheme_inputs.flat_frame_angular = control_scheme_inputs.flat_frame_linear;
        control_scheme_inputs.endpoint_frame_angular = control_scheme_inputs.endpoint_frame_linear;
        control_scheme_inputs.ik_angular = control_scheme_inputs.ik_linear;
    }

    return (joystick_r.btn_bottom_r6_state==1);
}

void JoystickTranslate::get_end_effector_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs, arm_interfaces::msg::EndEffectorInput& end_effector_inputs)
{
    if (!control_scheme_inputs.input_lock){
        // Set the values for linear actuator and end effector actuation
        end_effector_inputs.linear_actuation = joystick_l.ax_thumb_x;
        end_effector_inputs.end_effector_actuation = joystick_r.ax_thumb_x * 0.95;
        // Plans on moving the laser and hex control to here
        // end_effector_inputs.laser = ;
        // end_effector_inputs.hex_key = ;
        // end_effector_inputs.finger_actuation = ;
    } else {
        end_effector_inputs.linear_actuation = 0;
        end_effector_inputs.end_effector_actuation = 0;
    }

}

void JoystickTranslate::get_joint_velocity_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs, sensor_msgs::msg::JointState& joint_velocity_inputs)
{
    float speed = scale_speed(joystick_r.ax_slider) * speed_multipliers.all_inputs;
    joint_velocity_inputs.velocity.clear();
    if (!control_scheme_inputs.input_lock && !control_scheme_inputs.ik_linear) {
        // No speed scaling for lower joints;
        
        // Base rotation is stick twist. CCW rotates arm CCW (from above)
        joint_velocity_inputs.velocity.push_back(speed * joystick_l.ax_stick_twist);
        // Shoulder is stick y (left-right). Left moves the arm towards the back of the rover
        joint_velocity_inputs.velocity.push_back(speed * joystick_l.ax_stick_y);
        // Elbow is stick x (forward-backward). Forward pitches arm down
        joint_velocity_inputs.velocity.push_back(speed * -joystick_l.ax_stick_x);
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
        
        // J4 is stick x. Forward pitches arm down
        joint_velocity_inputs.velocity.push_back(speed_wrist_joints * -joystick_r.ax_stick_x);
        // J5 is stick y. Left yaws arm left
        joint_velocity_inputs.velocity.push_back(speed_wrist_joints * joystick_r.ax_stick_y);
        // J6 is stick twist. CCW tilts end effector CCW (looking out from end effector)
        joint_velocity_inputs.velocity.push_back(speed_wrist_joints * -joystick_r.ax_stick_twist);
    }
    else{
        joint_velocity_inputs.velocity.push_back(0);
        joint_velocity_inputs.velocity.push_back(0);
        joint_velocity_inputs.velocity.push_back(0);
    }
}

void JoystickTranslate::get_twist_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs, geometry_msgs::msg::TwistStamped& twist_inputs)
{
    float speed = scale_speed(joystick_r.ax_slider) * speed_multipliers.all_inputs;
    
    // If using lower joints IK, set the values for linear velocity
    if (!control_scheme_inputs.input_lock && control_scheme_inputs.ik_linear) {
        // Scale speed for linear IK
        float speed_ik_linear = speed * speed_multipliers.ik_linear;

        // Linear velocities map directly from joystick. Directions are already in arm base coords
        twist_inputs.twist.linear.x = speed_ik_linear * joystick_l.ax_stick_x;
        twist_inputs.twist.linear.y = speed_ik_linear * joystick_l.ax_stick_y;
        twist_inputs.twist.linear.z = speed_ik_linear * joystick_l.ax_stick_twist;
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
        // Roll is stick y (left-right)
        twist_inputs.twist.angular.x = speed_ik_angular * -joystick_r.ax_stick_y;
        // Pitch is stick x (forward-backward)
        twist_inputs.twist.angular.y = speed_ik_angular * joystick_r.ax_stick_x;
        // Yaw is stick twist
        twist_inputs.twist.angular.z = speed_ik_angular * joystick_r.ax_stick_twist;
    }
    else{
        twist_inputs.twist.angular.x = 0;
        twist_inputs.twist.angular.y = 0;
        twist_inputs.twist.angular.z = 0;
    }
}

float JoystickTranslate::scale_speed (float value){
    // Max scale factor 1.00, min scale factor 0.01
    return (value * value * 0.99) + 0.01;
}

bool JoystickTranslate::is_connected()
{
    return joystick_l.connected && joystick_r.connected;
}


void JoystickTranslate::joystick_l_callback(input_interfaces::msg::InputJoystick::SharedPtr msg){
    joystick_l = *msg;
}

void JoystickTranslate::joystick_r_callback(input_interfaces::msg::InputJoystick::SharedPtr msg){
    joystick_r = *msg;
}

void JoystickTranslate::reset_message()
{
    joystick_l = input_interfaces::msg::InputJoystick();
    joystick_r = input_interfaces::msg::InputJoystick();
}
