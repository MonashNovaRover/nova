#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This file defines build-time parameters for configuraing the arm software
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: None
TOPICS: None
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S):   Jory Braun
CREATION:	 23/04/2022
EDITED:		 23/04/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Item One
 - Item Two
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

namespace ArmConfig
{
    // Define all wrist types
    // These represent the physical assemblies that attach at the output of the elbow joint
    typedef enum {
        WRIST_CYCLOIDAL,
        WRIST_SPM
    } WristType;

    // Define all end effector types
    // These represent the physical assemblies that attach at the output of the wrist
    typedef enum {
        EE_EQUIPMENT_SERVICING,
        EE_EXTREME_RETRIEVAL
    } EndEffectorType;
    
    // Set the variables here to one of the options above to build the arm for whatever hardware is attached
    const WristType wrist_type = WRIST_CYCLOIDAL;
    const EndEffectorType end_effector_type = EE_EQUIPMENT_SERVICING;
}
