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

std::ostream& operator << (std::ostream& o, BLCMDTelemetry& tel) {
    o << "Rotor Velocity    : " << tel.rotor_velocity << std::endl <<
      "Q Current         : " << tel.q_current << std::endl <<
      "Rotor Interval    : " << tel.rotor_interval << std::endl <<
      "D Current         : " << tel.d_current << std::endl <<
      "Resolver Position : " << tel.resolver_position << std::endl <<
      "Resolver Velocity : " << tel.resolver_velocity << std::endl <<
      "Power             : " << tel.power << std::endl <<
      "Voltage           : " << tel.voltage << std::endl <<
      "Temperature       : " << tel.temp << std::endl;
    return o;
}

BLCMD::BLCMD (const std::string bus, const int id, BLCMDSendCommand drive_mode, const bool direction, BLCMDSendCommand stop_mode, double scaling_factor) :
    bus(bus), id(id), drive_mode(drive_mode), direction(direction), stop_mode(stop_mode), scaling_factor(scaling_factor), already_stopped(false)
{    
    // Set max speed
    max_speed = 1 / scaling_factor;
    // Set min speed
    min_speed = max_speed / 32767;

    // Set up the CAN interface with the correct bus
    can_bus = org::jcan::new_bus();
    std::cout << "BLCMD Initialised with id:" << id << ", with drive mode: "  << (drive_mode == DRIVE_VELOCITY ? "DRIVE_VELOCITY" : "DRIVE_POSITION") << std::endl;


    // Set up the CAN ID filter
    can_bus->set_id_filter({make_can_id(PACKET_1), make_can_id(PACKET_2),
                            make_can_id(PACKET_3), make_can_id(PACKET_4)});
    // Set up the CAN callbacks
    can_bus->add_callback_to(make_can_id(PACKET_1),this, &BLCMD::packet1_callback);
    can_bus->add_callback_to(make_can_id(PACKET_2), this, &BLCMD::packet2_callback);
    can_bus->add_callback_to(make_can_id(PACKET_3), this, &BLCMD::packet3_callback);
    can_bus->add_callback_to(make_can_id(PACKET_4), this, &BLCMD::packet4_callback);

    // Open the CAN bus
    can_bus->open(&bus[0]);
}


BLCMD::~BLCMD ()
{
    // Stop the BLCMD, safely close the socket
    stop();
    //can_socket.close();
}

void BLCMD::spin() {
        can_bus->spin();
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
    org::jcan::Frame frame;
    frame.id = make_can_id(command);
    // Write the frame to the bus
    can_bus->send(frame);
}


void BLCMD::stop ()
{
    switch(stop_mode){
        case STOP:
            write_frame_no_data(BLCMDSendCommand::STOP);
            break;
        case DRIVE_VELOCITY: drive(0.0);
            break;
        default:
            drive(0.0);
    }
}


void BLCMD::forward ()
{
    write_frame_no_data(BLCMDSendCommand::FORWARD);
}


void BLCMD::reverse ()
{
    write_frame_no_data(BLCMDSendCommand::REVERSE);
}


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
    // #TODO: Check if this works

//   //  If the motor has already been stopped do not send more stop commands
//   if (value == 0.0 && drive_mode != DRIVE_POSITION) {
//       if (already_stopped) {
//           return;
//       } else {
//           already_stopped = true;
//       }
//   } else {
//       already_stopped = false;
//    }

    // If the BLCMD is in position mode, scale the input value to radians
    if (drive_mode == DRIVE_POSITION){
        // map (-π,π) → (-1,1)
        value = value / position_factor;
    }
    else {
        if (scaling_factor != 1) {
            value *= scaling_factor;
        }
    }

    // Reverse the direction of the motor if necessary
    if (direction) {
        value *= -1;
    }

    // Saturate the input velocity if it is out of range
    if (value > 1) {
        value = 1;
    }
    else if (value < -1){
        value = -1;
    }

    // Create a new CAN frame
    org::jcan::Frame frame;
    frame.id = (id << 4) | drive_mode;

    // Scale the speed to the range
    int16_t scaled_value = convert_to_int16(value);

    // Order data in big-endian order (MSB first)
    frame.data.push_back(scaled_value >> 8);
    frame.data.push_back(scaled_value & 0xFF);

    // Write the frame to the bus
    can_bus->send(frame);
}



void BLCMD::packet1_callback(org::jcan::Frame frame) {
    telemetry.rotor_velocity = (direction ? -1 : 1 ) * int16_bytes_to_double(&frame.data[0]);
    telemetry.q_current = int16_bytes_to_double(&frame.data[2]);
}

void BLCMD::packet2_callback(org::jcan::Frame frame) {
    this->telemetry.rotor_interval = uint16_bytes_to_double(&frame.data[0]);
    this->telemetry.d_current = int16_bytes_to_double(&frame.data[2]);
}

void BLCMD::packet3_callback(org::jcan::Frame frame) {
    this->telemetry.resolver_position = int16_bytes_to_double(&frame.data[0]) * position_factor;
    this->telemetry.resolver_velocity = int16_bytes_to_double(&frame.data[2]) * velocity_factor;
}

void BLCMD::packet4_callback(org::jcan::Frame frame) {
    this->telemetry.power = uint16_bytes_to_double(&frame.data[0]);
    this->telemetry.voltage = uint16_bytes_to_double(&frame.data[2]);
    this->telemetry.temp = uint16_bytes_to_double(&frame.data[4]);
}

BLCMDTelemetry BLCMD::get_telemetry() {
    return this->telemetry;
}

int16_t BLCMD::convert_to_int16 (const double value) {
    // Convert the value to an integer
    return (int16_t)(value * 32767.0f);
}

int16_t BLCMD::from_bytes(uint8_t *bytes) {
    return (bytes[0] << 8) | bytes[1];
}

double BLCMD::int16_bytes_to_double (uint8_t* bytes)
{
    // Scale the value to a double
    return from_bytes(bytes)/32767.0;
}

double BLCMD::uint16_bytes_to_double (uint8_t* bytes)
{
    // Scale the value to a double
    return from_bytes(bytes)/65535.0;
}

uint16_t BLCMD::make_can_id(BLCMDSendCommand command)
{
    return SEND << 8 | id << 4 | command;
}

uint16_t BLCMD::make_can_id(BLCMDReceiveCommand command)
{
    return RECEIVE << 8 | id << 4 | command;
}

uint16_t BLCMD::make_can_id(TelemetryPacket packet)
{
    return RECEIVE << 8 | id << 4 | packet;
}

/// #TODO: Implement these functions with JCAN 1.8 and in a more elegant way after ARCh

//bool BLCMD::home_rotor()
//{
//    can_bus->set_id_filter({make_can_id(ERR_WARN_INF)});
//    write_frame_no_data(HOME_ROTOR);
//    Frame confirm_frame = can_bus->receive();
//
//    for(;;) {
//        if (!confirm_frame.data[0] && confirm_frame.data[1] == 8) return false;
//        if (confirm_frame.data[0] == 2 && confirm_frame.data[1] == 3) return true;
//        confirm_frame = can_bus->receive();
//    }
//}
//
//bool BLCMD::zero_resolver()
//{
//    can_bus->set_id_filter({make_can_id(ERR_WARN_INF)});
//    write_frame_no_data(ZERO_RESOLVER);
//    Frame confirm_frame = can_bus->receive();
//
//    for(;;) {
//        if (!confirm_frame.data[0] && (confirm_frame.data[1] == 8 ||
//            confirm_frame.data[1] == 4)) return false;
//        if (confirm_frame.data[0] == 2 && confirm_frame.data[1] == 4) return true;
//        confirm_frame = can_bus->receive();
//    }
//}

//void BLCMD::get_config_variable(ConfigVar var, BLCMDConfig *config)
//{
//    //create request can fram
//    Frame config_request;
//    config_request.id = make_can_id(GET_CONFIG);
//    config_request.data.push_back(var);
//
//    //set filter ID
//    can_bus->set_id_filter({make_can_id(CONFIG_DATA)});
//
//    //send data
//    can_bus->send(config_request);
//
//    //receive from config data
//    Frame config_data = can_bus->receive();
//
//    if(config_data.data[0] == var) {
//        switch (var) {
//            case HAS_RESOLVER:
//                config->has_resolver = config_data.data[1];
//                break;
//            case KP_CURRENT_LOOP:
//                config->kp_current_loop = from_bytes(&config_data.data[1]);
//                break;
//            case KI_CURRENT_LOOP:
//                config->ki_current_loop = from_bytes(&config_data.data[1]);
//                break;
//            case MAX_CURRENT_THRESHOLD:
//                config->max_current_threshold = from_bytes(&config_data.data[1]);
//                break;
//            case KP_VELOCITY_LOOP:
//                config->kp_velocity_loop = from_bytes(&config_data.data[1]);
//                break;
//            case KI_VELOCITY_LOOP:
//                config->ki_velocity_loop = from_bytes(&config_data.data[1]);
//                break;
//            case MAX_VELOCITY_THRESHOLD:
//                config->max_velocity_threshold = from_bytes(&config_data.data[1]);
//                break;
//            case MIN_INTERVAL:
//                config->min_interval = from_bytes(&config_data.data[1]);
//                break;
//            case KP_POSITION_LOOP:
//                config->kp_position_loop = from_bytes(&config_data.data[1]);
//                break;
//            case KI_POSITION_LOOP:
//                config->ki_position_loop = from_bytes(&config_data.data[1]);
//                break;
//            case MAX_POSITION_THRESHOLD:
//                config->max_position_threshold = from_bytes(&config_data.data[1]);
//                break;
//            case TELEMETRY_P1_SPEED:
//                config->telemetry_p1_speed = config_data.data[1];
//                break;
//            case TELEMETRY_P2_SPEED:
//                config->telemetry_p2_speed = config_data.data[1];
//                break;
//            case TELEMETRY_P3_SPEED:
//                config->telemetry_p3_speed = config_data.data[1];
//                break;
//            case TELEMETRY_P4_SPEED:
//                config->telemetry_p4_speed = config_data.data[1];
//                break;
//        }
//    }
//}

//BLCMDConfig BLCMD::get_configuration()
//{
//    BLCMDConfig config;
//
//    get_config_variable(HAS_RESOLVER, &config);
//    get_config_variable(KP_CURRENT_LOOP, &config);
//    get_config_variable(KI_CURRENT_LOOP, &config);
//    get_config_variable(MAX_CURRENT_THRESHOLD, &config);
//    get_config_variable(KP_VELOCITY_LOOP, &config);
//    get_config_variable(KI_VELOCITY_LOOP, &config);
//    get_config_variable(MAX_VELOCITY_THRESHOLD, &config);
//    get_config_variable(MIN_INTERVAL, &config);
//    get_config_variable(KP_POSITION_LOOP, &config);
//    get_config_variable(KI_POSITION_LOOP, &config);
//    get_config_variable(MAX_POSITION_THRESHOLD, &config);
//    get_config_variable(TELEMETRY_P1_SPEED, &config);
//    get_config_variable(TELEMETRY_P2_SPEED, &config);
//    get_config_variable(TELEMETRY_P3_SPEED, &config);
//    get_config_variable(TELEMETRY_P4_SPEED, &config);
//
//	return config;
//}

//bool BLCMD::set_config_variable(ConfigVar var, int16_t value)
//{
//    //create frame
//    Frame config_frame;
//    config_frame.id = make_can_id(SET_CONFIG);
//    config_frame.data.push_back(var);
//	config_frame.data.push_back(value);
//
//    can_bus->set_id_filter({make_can_id(WRITE_CONFIRMATION)});
//
//    can_bus->send(config_frame);
//
//    Frame confirmation_frame = can_bus->receive();
//
//    return confirmation_frame.data[0] == var ? confirmation_frame.data[1] : false;
//}
//
//bool BLCMD::set_config(BLCMDConfig *config)
//{
//    return set_config_variable(HAS_RESOLVER, config->has_resolver) ||
//    set_config_variable(KP_CURRENT_LOOP, config->kp_current_loop) ||
//    set_config_variable(KI_CURRENT_LOOP, config->ki_current_loop) ||
//    set_config_variable(MAX_CURRENT_THRESHOLD, config->max_current_threshold) ||
//    set_config_variable(KP_VELOCITY_LOOP, config->kp_velocity_loop) ||
//    set_config_variable(KI_VELOCITY_LOOP, config->ki_velocity_loop) ||
//    set_config_variable(MAX_VELOCITY_THRESHOLD, config->max_velocity_threshold) ||
//    set_config_variable(MIN_INTERVAL, config->min_interval) ||
//    set_config_variable(KP_POSITION_LOOP, config->kp_position_loop) ||
//    set_config_variable(KI_POSITION_LOOP, config->ki_position_loop) ||
//    set_config_variable(MAX_POSITION_THRESHOLD, config->max_position_threshold) ||
//    set_config_variable(TELEMETRY_P1_SPEED, config->telemetry_p1_speed) ||
//    set_config_variable(TELEMETRY_P2_SPEED, config->telemetry_p2_speed) ||
//    set_config_variable(TELEMETRY_P3_SPEED, config->telemetry_p3_speed) ||
//    set_config_variable(TELEMETRY_P4_SPEED, config->telemetry_p4_speed);
//}
