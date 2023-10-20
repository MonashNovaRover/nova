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
High: Make the set_message function take in a message type
    instead of a variadic template as the non typed 
    arguments is not safe
Low: Should std::any be used here? or the shared_ptr<void>?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "common_input_collections.h"
#include <memory>

class InputDevice {
    //------------------------------------------------------------//
    public:

    /// @brief returns the inputs for locking devices and joint limits
    virtual CommonInputCollections::ControlSchemeInputs get_arm_lock_inputs() = 0;

    /// @brief returns the inputs for the control scheme
    virtual CommonInputCollections::ControlSchemeInputs get_control_scheme_inputs() = 0;
    
    /// @brief returns the inputs for the end effector
    virtual CommonInputCollections::EndEffectorInputs get_end_effector_inputs() = 0;
    
    /// @brief returns the inputs for the joint velocities
    virtual CommonInputCollections::JointVelocityInputs get_joint_velocity_inputs() = 0;
    
    /// @brief returns the inputs for the twist
    virtual CommonInputCollections::TwistInputs get_twist_inputs() = 0;

    /// @brief returns whether the device is connected
    virtual bool is_connected() = 0;

    // Should take argument type of any message share ptr, but I cannot find the base class for that 

    /// @brief      Sets the message to the input device
    virtual void set_message(std::shared_ptr<void> msg, int idx) = 0;
    
    /// @brief     Resets the message of the input device
    virtual void reset_message() = 0;
};
