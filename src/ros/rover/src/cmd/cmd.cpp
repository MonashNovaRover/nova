/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios, Josh Cherubino, Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "cmd.h"
#include "print/print.h"

#define _USE_MATH_DEFINES
#include <cmath>


CMDOutputParameters::CMDOutputParameters (float reduction, int ppr, float velocity_factor, float clock_frequency) :
    using_output_parameters(true), reduction(reduction), ppr(ppr), velocity_factor(velocity_factor), clock_frequency(clock_frequency)
{
    command_scale_factor = 4 * ppr * reduction * velocity_factor / (M_PI * clock_frequency);
}


CMD::CMD (const int bus, const int id, CMDCommand drive_mode, CMDCommand stop_mode, const bool direction, CMDOutputParameters output_parameters) :
    bus(bus), id(id), drive_mode(drive_mode), stop_mode(stop_mode), direction(direction), output_parameters(output_parameters), already_stopped(false)
{
    // Set up the CAN interface with the correct bus
    scpp::SocketCanStatus status = can_socket.open(
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


CMD::~CMD ()
{
    // Stop the CMD, safely close the socket
    drive(0.0);
    can_socket.close();
}


int CMD::get_id()
{
    return id;
}


void CMD::write_frame_no_data (const CMDCommand command)
{
    // Create a new CAN frame
    scpp::CanFrame frame;
    frame.id = (id << 4) | command;
    frame.len = 0;

    // Write the frame to the bus
    can_socket.write(frame);
}


void CMD::stop ()
{
    write_frame_no_data(CMDCommand::STOP);
}


void CMD::forward ()
{
    write_frame_no_data(CMDCommand::FORWARD);
}


void CMD::reverse ()
{
    write_frame_no_data(CMDCommand::REVERSE);
}


void CMD::set_drive_mode (CMDCommand drive_mode)
{
    if (drive_mode == PWM || drive_mode == PID) {
        this->drive_mode = drive_mode;
    }
}


void CMD::set_stop_mode (CMDCommand stop_mode)
{
    if (stop_mode == STOP || stop_mode == PID) {
        this->stop_mode = stop_mode;
    }
}


void CMD::drive (float velocity)
{
    // Handle STOPs if set
    // Prevent needless repetition of STOPs (crowds the CAN bus, makes it hard to debug other things)
    if (stop_mode == STOP && velocity == 0) {
        if (!already_stopped) {
            stop();
            already_stopped = true;
        }
        return;
    }
    else {
        already_stopped = false;
    }

    // Scale physical velocity to an equivalent CMD command, which is the fraction of the CMDs max speed
    if (output_parameters.using_output_parameters) {
        velocity *= output_parameters.command_scale_factor;
    }

    // Saturate the input velocity if it is out of range
    if (velocity > 1.0) {
        velocity = 1.0;
    }
    else if (velocity < -1.0){
        velocity = -1.0;
    }
    
    // Flip output direction if needed
    if (direction){
        velocity *= -1;
    }

    // Create a new CAN frame
    scpp::CanFrame frame;
    frame.id = (id << 4) | drive_mode;
    frame.len = 2;

    // Scale the speed to the range
    int16_t scaled_velocity = convert_to_int16(velocity);

    // Order data in big-endian order (MSB first)
    frame.data[0] = scaled_velocity >> 8;
    frame.data[1] = scaled_velocity & 0xFF;

    // Write the frame to the bus
    can_socket.write(frame);    
}


void CMD::set_linear_actuator (float value)
{
    unsigned char actuation = 0;
    if (value < 0){
        actuation = 2;
    }
    else if (value > 0){
        actuation = 1;
    }

    // Create a new CAN frame
    scpp::CanFrame frame;
    frame.id = (id << 4) | CMDCommand::ACTUATOR;
    frame.len = 2;

    // Order data in big-endian order (MSB first)
    frame.data[0] = actuation;

    // Write the frame to the bus
    can_socket.write(frame);
}


void CMD::set_tuning_parameters (double kP, double kI, double kD, double kM)
{
    // Create a new CAN frame
    scpp::CanFrame frame;
    frame.id = (id << 4) | CMDCommand::TUNER;
    frame.len = 8;

    // Scale the constants
    int16_t scaled_kP = convert_to_int16(kP);
    int16_t scaled_kI = convert_to_int16(kI);
    int16_t scaled_kD = convert_to_int16(kD);
    int16_t scaled_kM = convert_to_int16(kM);

    // Construct the data
    frame.data[0] = scaled_kP >> 8;
    frame.data[1] = scaled_kP & 0xFF;
    frame.data[2] = scaled_kI >> 8;
    frame.data[3] = scaled_kI & 0xFF;
    frame.data[4] = scaled_kD >> 8;
    frame.data[5] = scaled_kD & 0xFF;
    frame.data[6] = scaled_kM >> 8;
    frame.data[7] = scaled_kM & 0xFF;

    // Write the frame to the bus
    can_socket.write(frame);
}


CMDData CMD::receive_feedback ()
{
    // Creates a new CAN frame
    scpp::CanFrame frame;
    frame.id = (id << 4) | CMDCommand::TUNER;
    frame.len = 4;

    // Read the data
    can_socket.read(frame);

    // Convert scaled data to the double
    double velocity = convert_from_bytes(frame.data);
    double power = convert_from_bytes(frame.data + 2);
    
    // Create a new struct
    CMDData data = CMDData(velocity, power);

    // Return the data
    return data;
}


int16_t CMD::convert_to_int16 (const double value)
{
    // Convert the value to an integer
    return (int16_t)(value * 32767.0f);
}


double CMD::convert_from_bytes (uint8_t* bytes)
{
    // Calculate the integer 16 value
    int16_t input = (bytes[0] << 8) | bytes[1];

    // Scale the value to a double
    return static_cast<double>(input) / 32767;
}
