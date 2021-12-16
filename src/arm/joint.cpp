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

Joint::Joint (const int id, int CMD_drive_mode) :
    CMD (1, id) { 

    // Update the variables
    this->CMD_drive_mode = CMD_drive_mode;
    
}


Joint::~Joint () {
    // Ensure the joint stops
    drive(0.0);
}


void Joint::drive (float velocity) {

    if(CMD_drive_mode){
        // Call the PID function
        set_pid(velocity);
    }
    else{
        // Call the PWM function
        set_pwm(velocity);
    }
    
}

void Joint::stop () {
    
    // Call base class stop
    CMD::stop();
}

void Joint::set_CMD_drive_mode (int CMD_drive_mode){
    this->CMD_drive_mode = CMD_drive_mode;
}