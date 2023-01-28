/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios, Josh Cherubino, Jory Braun, Taaj Street
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "blcmd.h"
#include "print/print.h"

#define _USE_MATH_DEFINES
#include <cmath>

using namespace org::jcan;
//TODO: change bus to enable vcan for testing purposes
BLCMD::BLCMD (const int bus, const int id, BLCMDSendCommand drive_mode, const bool direction, BLCMDSendCommand stop_mode, double scaling_factor) :
    bus(bus), id(id), drive_mode(drive_mode), direction(direction), stop_mode(stop_mode), scaling_factor(scaling_factor), already_stopped(false)
{    
    // Set max speed
    max_speed = 1 / scaling_factor;
    // Set min speed
    min_speed = max_speed / 32767;

    // Set up the CAN interface with the correct bus
    //TODO: Proper error handling
    can_bus = org::jcan::open_bus(
        (bus == 0) ? "can0" : "can1"
    ).into_raw();
}


BLCMD::~BLCMD ()
{
    // Stop the BLCMD, safely close the socket
    drive(0.0);
    //can_socket.close();
}


double BLCMD::get_scaling_factor(double reduction, int ppr, double velocity_factor, double clock_frequency)
{
    // Working for the following formula is given on Nuclino or in Jory Braun's FYP (section 8.1.7)
    // https://app.nuclino.com/Nova-Rover-Team/Arm/Arm-Control-Hardware-36889286-3c59-431e-8860-fbecf757e00c
    // https://drive.google.com/drive/folders/1_Qy3f84bZfiX-lKIdvLG0Iklqw85T36b
    // The calculation excludes rounding errors or saturation
    return 4 * ppr * reduction * velocity_factor / (M_PI * clock_frequency);
}


int BLCMD::get_id()
{
    return id;
}


void BLCMD::write_frame_no_data (const BLCMDSendCommand command)
{
    // Create a new CAN frame
    Frame frame;
    frame.id = (id << 4) | command;
    frame.data.push_back(0x00000000);
    // Write the frame to the bus
    can_bus->send(frame);
}


void BLCMD::stop ()
{
    write_frame_no_data(BLCMDSendCommand::STOP);
}


void BLCMD::forward ()
{
    write_frame_no_data(BLCMDSendCommand::FORWARD);
}


void BLCMD::reverse ()
{
    write_frame_no_data(BLCMDSendCommand::REVERSE);
}

//TODO: Find
void BLCMD::set_drive_mode (BLCMDSendCommand drive_mode)
{
    if (drive_mode == DRIVE_VELOCITY || drive_mode == DRIVE_POSITION ||
        drive_mode == DRIVE_CURRENT  || drive_mode == DRIVE_OPEN_LOOP) {
        this->drive_mode = drive_mode;
    }
}


void BLCMD::set_stop_mode (BLCMDSendCommand stop_mode)
{
    if (stop_mode == STOP || stop_mode == DRIVE_VELOCITY) {
        this->stop_mode = stop_mode;
    }
}


void BLCMD::drive (float value)
{


    if (drive_mode == DRIVE_POSITION){
        // map (-π,π) → (-1,1)
        value = value / M_PI;
    }
    else {
        // Handle STOPs if set
        // Prevent needless repetition of STOPs (crowds the CAN bus, makes it hard to debug other things)
        // If using PWM, always use STOP
        if (value == 0 && (stop_mode == STOP || drive_mode == DRIVE_CURRENT)) {
            if (!already_stopped) {
                stop();
                already_stopped = true;
            }
            return;
        } else {
            already_stopped = false;
        }

        // Scale physical velocity to an equivalent BLCMD command, which is the fraction of the BLCMDs max speed
        if (scaling_factor != 1) {
            value *= scaling_factor;
        }


        // Flip output direction if needed
        if (direction) {
            value *= -1;
        }
    }

    // Saturate the input velocity if it is out of range

    if (value > 1.0) {
        value = 1.0;
    }
    else if (value < -1.0){
        value = -1.0;
    }

    // Create a new CAN frame
    Frame frame;
    frame.id = (id << 4) | drive_mode;

    // Scale the speed to the range
    int16_t scaled_value = convert_to_int16(value);

    // Order data in big-endian order (MSB first)
    frame.data.push_back(scaled_value >> 8);
    frame.data.push_back(scaled_value & 0xFF);

    // Write the frame to the bus
    can_bus->send(frame);
}


void BLCMD::set_linear_actuator (float value)
{
//    unsigned char actuation = 0;
//    if (value < 0){
//        actuation = 2;
//    }
//    else if (value > 0){
//        actuation = 1;
//    }
//
//    // Create a new CAN frame
//    Frame frame;
//    frame.id = (id << 4) | BLCMDSendCommand::ACTUATOR;
//
//    // Order data in big-endian order (MSB first)
//    frame.data.push_back(actuation);
//
//    // Write the frame to the bus
//    can_bus->send(frame);
}


void BLCMD::set_tuning_parameters (double kP, double kI, double kD, double kM)
{
//    // Create a new CAN frame
//    scpp::CanFrame frame;
//    frame.id = (this->id << 4) | BLCMDSendCommand::SET_TUNE;
//    frame.len = 8;
//
//    // Scale the constants
//    int16_t scaled_kP = convert_to_int16(kP);
//    int16_t scaled_kI = convert_to_int16(kI);
//    int16_t scaled_kD = convert_to_int16(kD);
//    int16_t scaled_kM = convert_to_int16(kM);
//
//    // Construct the data
//    frame.data[0] = scaled_kP >> 8;
//    frame.data[1] = scaled_kP & 0xFF;
//    frame.data[2] = scaled_kI >> 8;
//    frame.data[3] = scaled_kI & 0xFF;
//    frame.data[4] = scaled_kD >> 8;
//    frame.data[5] = scaled_kD & 0xFF;
//    frame.data[6] = scaled_kM >> 8;
//    frame.data[7] = scaled_kM & 0xFF;
//
//    // Write the frame to the bus
//    can_socket.write(frame);
}

//TODO: Make better return value.
BLCMDTelemetry BLCMD::get_telemetry_packet(TelemetryPacket packet_num)
{
    can_bus->set_id_filter({4 << 8 | id << 4 | packet_num});
    Frame packet = can_bus->receive();

    BLCMDTelemetry telemetry;

    switch (packet_num){
        case PACKET_1:
            telemetry.rotor_velocity = bytes_to_int16(&packet.data[0]);
            telemetry.q_current = bytes_to_int16(&packet.data[2]);
            break;
        case PACKET_2:
            telemetry.rotor_interval = bytes_to_int16(&packet.data[0]);
            telemetry.d_current = bytes_to_int16(&packet.data[2]);
            break;
        case PACKET_3:
            telemetry.resolver_position = bytes_to_int16(&packet.data[0]);
            telemetry.resolver_velocity = bytes_to_int16(&packet.data[2]);
            break;
        case PACKET_4:
            telemetry.power = bytes_to_int16(&packet.data[0]);
            telemetry.voltage = bytes_to_int16(&packet.data[2]);
            telemetry.temp = bytes_to_int16(&packet.data[4]);
            break;
    }

    return telemetry;
}


int16_t BLCMD::convert_to_int16 (const double value) {
    // Convert the value to an integer
    return (int16_t)(value * 32767.0f);
}

int16_t BLCMD::bytes_to_int16 (uint8_t* bytes)
{
    return (bytes[0] << 8) | bytes[1];
}