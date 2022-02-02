/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios, Josh Cherubino
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// General includes
#include <iostream>

// Include the header file
#include "wheel.h"

Wheel::Wheel (const int id, const bool clockwise) :
    CMD (0, id) {

    // Update the variables
    this->clockwise = clockwise;
    
    //TODO: Send PID gains on startup...
}


Wheel::~Wheel () {
    // Ensure the spinning stops
    spin(0.0);
}


//TODO: Add mode as parameter to function (i.e. PID etc.)
void Wheel::spin (float speed) {

    // Adjust for directional spinning
    if (this->clockwise) speed *= -1.0;

    // Make sure the limits on speed
    if (speed > 1.0) speed = 1.0;
    else if (speed < -1.0) speed = -1.0;

    // Call the PID function
    set_pid(speed);
}


void Wheel::spin (float speed, const float steer) {

    // Calculate the new speed based on the steer
    if (clockwise)
        speed = speed + steer;
    else
        speed = speed - steer;

    // Call the base spin function
    spin (speed);
}


void Wheel::stop () {
    
    // Call base class stop
    CMD::stop();
}
