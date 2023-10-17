#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class is an interface for the input devices
and converts them to arm input messages.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Matthew Gu
CREATION:	23/09/2023
EDITED:		30/09/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "input_device.h"
#include "core/msg/input_joystick.hpp"

class JoystickTranslate: public InputDevice {
    //------------------------------------------------------------//
    private:
    core::msg::InputJoystick joystick_l;
    core::msg::InputJoystick joystick_r;

    ControlSchemeInputs control_scheme_inputs;
    EndEffectorInputs end_effector_inputs;
    JointVelocityInputs joint_velocity_inputs;
    TwistInputs twist_inputs;

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

    JoystickTranslate();

    ControlSchemeInputs get_arm_lock_inputs() override;

    ControlSchemeInputs get_control_scheme_inputs() override;

    EndEffectorInputs get_end_effector_inputs() override;

    JointVelocityInputs get_joint_velocity_inputs() override;

    TwistInputs get_twist_inputs() override;

    bool is_connected();

    void set_message(Message msg, int idx);
    
    void reset_message();
};
