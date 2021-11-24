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
    
    //startup CAN interface.
    this->can_socket.open("can1");
    
    //TODO: Send PID gains on startup...
}


Wheel::~Wheel () {
    // Ensure the spinning stops
    spin(0.0);
    this->can_socket.close();
}


//TODO: Add mode as parameter to function (i.e. PID etc.)
void Wheel::spin (float speed) {

    // Adjust for directional spinning
    if (!this->clockwise) speed *= -1.0;

    // Make sure the limits on speed
    if (speed > 1.0) speed = 1.0;
    else if (speed < -1.0) speed = -1.0;

    //Send data on wire
    scpp::CanFrame frame;
    
    //embed command in arbitration id
    frame.id = (this->id << 4) | SET_VELOCITY;
    
    //scale to range
    int16_t scaled_speed = (int16_t)(speed*32767.0f);

    //Order data in big-endian order (MSB first)
    frame.data[0] = scaled_speed >> 8;
    frame.data[1] = scaled_speed & 0xFF; 
    frame.len = 2;
    
    this->can_socket.write(frame);
}


void Wheel::spin (float speed, const float steer) {

    // Calculate the new speed based on the steer
    speed = speed + (steer * ((clockwise) ? 1.0 : -1.0));

    // Call the base spin function
    spin (speed);
}
