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
- Make the set_message function take in a message type
    instead of a variadic template as the non typed 
    arguments is not safe
- Make the set_message index to be some enum or something
    instead of an int
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "common_input_collections.h"
#include <cstdint>
#include <memory>
#include <vector>

class InputDevice {
    //------------------------------------------------------------//
    public:

    virtual CommonInputCollections::ControlSchemeInputs get_arm_lock_inputs() = 0;
    virtual CommonInputCollections::ControlSchemeInputs get_control_scheme_inputs() = 0;
    virtual CommonInputCollections::EndEffectorInputs get_end_effector_inputs() = 0;
    virtual CommonInputCollections::JointVelocityInputs get_joint_velocity_inputs() = 0;
    virtual CommonInputCollections::TwistInputs get_twist_inputs() = 0;
    virtual bool is_connected() = 0;
    // Should take argument type of any message share ptr, but I cannot find the base class for that 
    template<typename Message>
    virtual void set_message(Message msg, int idx) = 0;
    virtual void reset_message() = 0;
};
