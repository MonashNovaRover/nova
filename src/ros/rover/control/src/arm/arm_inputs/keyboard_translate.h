#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class is responsible for translating keyboard
messages into common arm input messages.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Matthew Gu
CREATION:	23/09/2023
EDITED:		30/09/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
Ask operator for key mapping
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "input_device.h"
#include "common_input_collections.h"
#include "core/msg/input_keyboard.hpp"

#include <memory>

class KeyboardTranslate: public InputDevice {
    //------------------------------------------------------------//
    private:
    core::msg::InputKeyboard keyboard;

    static float speed_increment = 0.05;
    float joint_twist_speed;
    float end_effector_speed;

    typedef struct {
        // Multiplier for all inputs
        // Tune this to adjust the max velocity of all joints
        float all_inputs = 0.30;
        // Separate multipliers for each set of inputs
        // Tune these so joints move at reasonable speeds relative to each other
        float wrist_joints = 1.20;
        float ik_linear = 0.50;
        float ik_angular = 0.85;
    } SpeedMultipliers;
    SpeedMultipliers speed_multipliers;

    CommonInputCollections::ControlSchemeInputs control_scheme_inputs;
    CommonInputCollections::EndEffectorInputs end_effector_inputs;
    CommonInputCollections::JointVelocityInputs joint_velocity_inputs;
    CommonInputCollections::TwistInputs twist_inputs;

    bool is_released(int key);
    bool is_pressed(int key);
    bool is_held(int key);
    bool is_pressed_or_held(int key);
    bool is_ctrl();
    bool is_shift();

    float increase_speed(float value);
    float decrease_speed(float value);
    //------------------------------------------------------------//
    public:

    KeyboardTranslate();

    CommonInputCollections::ControlSchemeInputs get_arm_lock_inputs() override;

    CommonInputCollections::ControlSchemeInputs get_control_scheme_inputs() override;

    CommonInputCollections::EndEffectorInputs get_end_effector_inputs() override;

    CommonInputCollections::JointVelocityInputs get_joint_velocity_inputs() override;

    CommonInputCollections::TwistInputs get_twist_inputs() override;

    void set_message(std::shared_ptr<void> msg, int idx);

    void reset_message();

    bool is_connected();
};
