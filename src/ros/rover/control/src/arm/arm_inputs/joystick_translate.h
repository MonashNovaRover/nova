#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class is responsible for translating joystick
messages into common arm input messages.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Jess Hepworth, Jory Braun
MODIFIED:	Matthew Gu
CREATION:	09/10/2023
EDITED:		07/12/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "input_device.h"
#include "common_input_collections.h"
#include "core/msg/input_joystick.hpp"

#include <memory>

class JoystickTranslate: public InputDevice {
    //------------------------------------------------------------//
    private:
    // store the messages
    core::msg::InputJoystick joystick_l;
    core::msg::InputJoystick joystick_r;

    // store the inputs to be returned
    CommonInputCollections::ControlSchemeInputs control_scheme_inputs;
    CommonInputCollections::EndEffectorInputs end_effector_inputs;
    CommonInputCollections::JointVelocityInputs joint_velocity_inputs;
    CommonInputCollections::TwistInputs twist_inputs;

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

    /// @brief      Obtains postive scaling factor from slider input
    /// @param      value - number in range [-1, 1] to map to [0, 1]
    /// @returns    The new scale factor in range [0, 1]
    float scale_speed(float value);

    //------------------------------------------------------------//
    public:

    /// @brief      Constructor for the joystick translator
    JoystickTranslate();

    // See input_device.h for documentation
    CommonInputCollections::ControlSchemeInputs get_control_scheme_inputs() override;

    CommonInputCollections::EndEffectorInputs get_end_effector_inputs() override;

    CommonInputCollections::JointVelocityInputs get_joint_velocity_inputs() override;

    CommonInputCollections::TwistInputs get_twist_inputs() override;

    bool is_connected() override;

    /// @brief  Callback for the left joystick message
    /// @param msg - the joystick message 
    void joystick_l_callback(core::msg::InputJoystick::SharedPtr msg);

    /// @brief  Callback for the right joystick message
    /// @param msg - the joystick message
    void joystick_r_callback(core::msg::InputJoystick::SharedPtr msg);
    
    void reset_message() override;
};
