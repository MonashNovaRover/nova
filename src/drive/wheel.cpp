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

Wheel::Wheel (const int id, const bool left) :
    CMD (0, id) {

    // Update the variables
    this->left = left;
    
    //TODO: Send PID gains on startup...
}


Wheel::~Wheel () {
    // Ensure the spinning stops
    spin(0.0);
}


// TODO: Add mode as parameter to function (i.e. PID etc.)
void Wheel::spin (float speed) {

    // Adjust for directional spinning
    if (!this->left) speed *= -1.0;

    // Make sure the limits on speed
    if (speed > 1.0) speed = 1.0;
    else if (speed < -1.0) speed = -1.0;

    // Call the PID function
    set_pid(speed);
}


void Wheel::spin (float speed, const float steer) {

    // If not turning
    if (steer == 0) {
        spin(speed);
        return;
    }

    // TODO with speed

    // Call the base spin function
    spin (speed);
}


void Wheel::stop () {
    // Call base class stop
    CMD::stop();
}


int Wheel::get_id () {
    return this->id;
}

