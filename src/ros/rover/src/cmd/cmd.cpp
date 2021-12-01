/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios, Josh Cherubino
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "cmd.h"


CMD::CMD (const int bus, const int id) {

    // Update the parameters of the CMD
    this->bus = bus;
    this->id = id;

    // Set up the CAN interface with the correct bus
    this->can_socket.open(
        (bus == 0) ? "can0" : "can1"
    );
}


CMD::~CMD () {
    // Safely close the sockets
    this->can_socket.close();
}


void CMD::stop () {

    // Creates a new CAN frame
    scpp::CanFrame frame;
    frame.id = (this->id << 4) | CMDCommand::STOP;
    frame.len = 0;

    // Writes the frame
    this->can_socket.write(frame);
}


void CMD::set_pwm (float power) {

    // Creates a new CAN frame
    scpp::CanFrame frame;
    frame.id = (this->id << 4) | CMDCommand::PWM;
    frame.len = 2;

    // Scale the power to the range
    int16_t scaled_power = (int16_t)(power * 32767.0f);

    // Order data in big-endian order (MSB first)
    frame.data[0] = scaled_power >> 8;
    frame.data[1] = scaled_power & 0xFF;

    // Write the frame
    this->can_socket.write(frame);
}


void CMD::set_pid (float speed) {

    // Creates a new CAN frame
    scpp::CanFrame frame;
    frame.id = (this->id << 4) | CMDCommand::PID;
    frame.len = 2;

    // Scale the speed to the range
    int16_t scaled_speed = (int16_t)(speed * 32767.0f);

    // Order data in big-endian order (MSB first)
    frame.data[0] = scaled_speed >> 8;
    frame.data[1] = scaled_speed & 0xFF;

    // Write the frame
    this->can_socket.write(frame);
}