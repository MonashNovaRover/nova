/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios, Josh Cherubino
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "cmd.h"
#include "debug/print.h"


CMD::CMD (const int bus, const int id) {

    // Update the parameters of the CMD
    this->bus = bus;
    this->id = id;

    // Set up the CAN interface with the correct bus
    scpp::SocketCanStatus status = this->can_socket.open(
        (bus == 0) ? "can0" : "can1"
    );

    // Check for status
    switch (status) {
        case scpp::STATUS_OK:
            Print::print("Initialised CAN device successfully.", C_SUCCESS); 
            break;
        default:
            if (bus == 0)   Print::print("Error: can0 has not been initialized.", C_FAIL);
            else            Print::print("Error: can1 has not been initialized.", C_FAIL);
            break;
    }
}


CMD::~CMD () {
    // Safely close the sockets
    this->can_socket.close();
}


void CMD::call_empty (const CMDCommand command) {

    // Creates a new CAN frame
    scpp::CanFrame frame;
    frame.id = (this->id << 4) | command;
    frame.len = 0;

    // Writes the frame
    this->can_socket.write(frame);
}


void CMD::stop () {

    // Calls the empty frame
    call_empty(CMDCommand::STOP);
}


void CMD::forward () {

    // Calls the empty frame
    call_empty(CMDCommand::FORWARD);
}


void CMD::reverse () {

    // Calls the empty frame
    call_empty(CMDCommand::REVERSE);
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

void CMD::set_linear_actuator (float value){
    int actuation = 0;
    if (value < 0){
        actuation = 2;
    }
    else if (value > 0){
        actuation = 1;
    }

    // Creates a new CAN frame
    scpp::CanFrame frame;
    frame.id = (this->id << 4) | CMDCommand::ACTUATOR;
    frame.len = 2;

    // Order data in big-endian order (MSB first)
    frame.data[0] = actuation >> 8;
    frame.data[1] = actuation & 0xFF;

    // Write the frame
    this->can_socket.write(frame);


}