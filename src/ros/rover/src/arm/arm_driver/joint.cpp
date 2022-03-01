/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jess Hepworth
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// General includes
#include <iostream>

// Include the header file
#include "joint.h"

Joint::Joint (const int id, CMDCommand CMD_drive_mode, const bool CMD_direction) :
    CMD (1, id) { 

    // Update the variables
    this->CMD_drive_mode = CMD_drive_mode;
    this->CMD_direction = CMD_direction;
    
}


Joint::~Joint () {
    // Ensure the joint stops
    drive(0.0);
}


void Joint::drive (float velocity) {
    velocity = reverse * velocity;

    // Check if velocity is zero
    if (velocity == 0) {
        // If not all stopped
        if (!all_stopped) {
            all_stopped = true;
            stop();
        }

        return;    
    }

    // Otherwise turn off all stops
    else all_stopped = false;
    
    // Flip output direction if needed
    if (CMD_direction){
        velocity *= -1;
    }

    if(CMD_drive_mode == PID){
        // Call the PID function
        set_pid(velocity);
    }
    else if (CMD_drive_mode == PWM) {
        // Call the PWM function
        set_pwm(velocity);
    }
    
}

void Joint::stop () {
    
    // Call base class stop
    CMD::stop();
}

void Joint::set_CMD_drive_mode (CMDCommand CMD_drive_mode){
    this->CMD_drive_mode = CMD_drive_mode;
}