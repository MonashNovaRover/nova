#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

ROS2 parameters and definitions for control nodes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S):   Jory Braun
CREATION:	 12/01/2023
EDITED:		 12/01/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include <chrono>

using namespace std::chrono_literals;

typedef std::chrono::milliseconds millis; 

namespace ROSTimers
{
    // Publisher timer periods
    const millis arm_visualisation = 10ms;
    const millis arm_resolvers = 10ms;
    const millis auto_mode = 200ms;
    const millis arm_startup_timer = 100ms;
    const millis arm_control = 10ms;
    const millis drive_control = 10ms;
    const millis blcmds_telemetry = 50ms;
    
    // Timers for legacy nodes
    const millis pid_tuner_control = 100ms;
    const millis pid_tuenr_feedback = 50ms;

    // Other timer periods
    const millis arm_deadline = 200ms;
    const millis drive_deadline = 200ms;
}
