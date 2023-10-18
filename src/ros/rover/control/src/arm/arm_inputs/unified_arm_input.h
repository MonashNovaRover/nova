#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class decides which input device to use

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Matthew Gu
CREATION:	23/09/2023
EDITED:		30/09/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
High: Swap JoyStick message to use Joy message type
Medium: Make joystick override changeable in launch file/runtime
Low: See if it is more appropriate to move the subscribes to the translator
Low: Make key mapping changeable in runtime (json config file)?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "input_device.h"
#include "core/msg/input_joystick.hpp"
#include "core/msg/input_keyboard.hpp"

class UnifiedArmInput {
    //------------------------------------------------------------//
    private:
    static const int NUM_INPUT_DEVICES = 2;

    bool joystick_override;

    // Define an enum for the input device indices
    enum InputDeviceIndex {
        JOYSTICK_INDEX = 0,
        KEYBOARD_INDEX = 1
    };

    // Store the input devices
    InputDevice input_devices [NUM_INPUT_DEVICES];
    
    /// @brief      Returns the input device to use
    InputDevice& get_input_device ();

    void deadline_callback (int device_index);

    //------------------------------------------------------------//
    public:

    /// @brief     Constructor for the ArmInputs class.
    UnifiedArmInput(bool joystick_override);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_l_callback (const core::msg::InputJoystick::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_r_callback (const core::msg::InputJoystick::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void keyboard_callback (const core::msg::InputKeyboard::SharedPtr msg);

    void joystick_deadline_callback ();

    void keyboard_deadline_callback ();

    /// @brief 
    /// @return 
    ControlSchemeInputs get_arm_lock_inputs();
    ControlSchemeInputs get_control_scheme_inputs();
    EndEffectorInputs get_end_effector_inputs();
    JointVelocityInputs get_joint_velocity_inputs();
    TwistInputs get_twist_inputs();
}
