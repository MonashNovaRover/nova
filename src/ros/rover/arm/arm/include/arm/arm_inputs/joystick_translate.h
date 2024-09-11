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
#include "input_interfaces/msg/input_joystick.hpp"


class JoystickTranslate: public InputDevice {
    //------------------------------------------------------------//
    private:
    // store the messages
    input_interfaces::msg::InputJoystick joystick_l;
    input_interfaces::msg::InputJoystick joystick_r;

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
    bool get_control_scheme_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs) override;

    void get_end_effector_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs, arm_interfaces::msg::EndEffectorInput& end_effector_inputs) override;

    void get_joint_velocity_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs, sensor_msgs::msg::JointState& joint_velocity_inputs) override;
    
    void get_twist_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs, geometry_msgs::msg::TwistStamped& twist_inputs) override;

    bool is_connected() override;

    /// @brief  Callback for the left joystick message
    /// @param msg - the joystick message 
    void joystick_l_callback(input_interfaces::msg::InputJoystick::SharedPtr msg);

    /// @brief  Callback for the right joystick message
    /// @param msg - the joystick message
    void joystick_r_callback(input_interfaces::msg::InputJoystick::SharedPtr msg);
    
    void reset_message() override;
};
