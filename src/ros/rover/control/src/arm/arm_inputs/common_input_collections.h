#pragma once
/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class is a collection of the arm inputs that
should be supported by any input device.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Matthew Gu
CREATION:	18/10/2023
EDITED:		18/10/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#include <cstdint>
class CommonInputCollections {    
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
};