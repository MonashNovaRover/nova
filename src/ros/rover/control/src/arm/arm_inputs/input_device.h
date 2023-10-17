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

#include <cstdint>
#include <memory>
#include <vector>

class InputDevice {
    //------------------------------------------------------------//
    public:

    typedef struct {
        float x; 
        float y; 
        float z;
    } Twist;

    // Stores the abstracted inputs the arm will use
    // control scheme inputs
    typedef struct {
        bool control_scheme_update;
        bool input_lock;
        bool joint_limits;
        bool position_control;

        int8_t base_frame_offset;
        bool flat_fram_linear;
        bool flat_frame_angular;
        bool endpoint_frame_linear;
        bool endpoint_frame_angular;
        bool ik_linear;
        bool ik_angular;
        bool use_spm_roll;
    } ControlSchemeInputs;
    

    // end effector inputs
    typedef struct{
        float linear_actuation;
        float end_effector_actuation;
    } EndEffectorInputs;
    
    // joint space inputs
    typedef struct {
        float velocities [6];
    } JointVelocityInputs;
    
        
    typedef struct{
        Twist linear;
        Twist angular;
    } TwistInputs;

    virtual ControlSchemeInputs get_arm_lock_inputs() = 0;
    virtual ControlSchemeInputs get_control_scheme_inputs() = 0;
    virtual EndEffectorInputs get_end_effector_inputs() = 0;
    virtual JointVelocityInputs get_joint_velocity_inputs() = 0;
    virtual TwistInputs get_twist_inputs() = 0;
    virtual bool is_connected() = 0;
    // Should take argument type of any message share ptr, but I cannot find the base class for that 
    template<typename Message>
    virtual void set_message(Message msg, int idx) = 0;
    virtual void reset_message() = 0;
};
