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
EDITED:		07/12/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_interfaces/msg/end_effector_input.hpp"
#include "arm_interfaces/msg/arm_control_scheme.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"

class InputDevice {
    //------------------------------------------------------------//
    public:

    /// @brief returns the inputs for the control scheme
    /// @returns 1 if toggling the input method, 0 if not
    virtual bool get_control_scheme_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs) = 0;
    
    /// @brief returns the inputs for the end effector
    virtual void get_end_effector_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs, arm_interfaces::msg::EndEffectorInput& end_effector_inputs) = 0;
    
    /// @brief returns the inputs for the joint velocities
    virtual void get_joint_velocity_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs, sensor_msgs::msg::JointState& joint_velocity_inputs) = 0;
    
    /// @brief returns the inputs for the twist
    virtual void get_twist_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs, geometry_msgs::msg::TwistStamped& twist_inputs) = 0;

    /// @brief returns whether the device is connected
    virtual bool is_connected() = 0;

    /// @brief     Resets the message of the input device
    virtual void reset_message() = 0;
};
