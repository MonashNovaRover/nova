/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "wheel.h"

Wheel::Wheel (const int id, const bool clockwise) {

    // Update the variables
    this->id = id;
    this->clockwise = clockwise;
}


Wheel::~Wheel () {
    // Ensure the spinning stops
    spin(0.0);
}


void Wheel::spin (float speed) {

    // Adjust for directional spinning
    if (!this->clockwise) speed *= -1.0;

    // Make sure the limits on speed
    if (speed > 1.0) speed = 1.0;
    else if (speed < -1.0) speed = -1.0;

    // TODO remove when CAN classes exists
    cout << "Wheel Spin: " << speed << endl;
    fflush(stdout);
}


void Wheel::spin (float speed, const float steer) {

    // Calculate the new speed based on the steer
    speed = speed + (steer * ((clockwise) ? 1.0 : -1.0));

    // Call the base spin function
    spin (speed);
}