#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This code interfaces with the CAN classes and is
    able to communicate with all of the BLCMD/PACMANs
    electronic code by creating instances of each class.

This code used the cmd class code as a base.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Taaj Street, Harrison Verrios, Josh Cherubino, Jory Braun
CREATION:	22/01/2023
EDITED:		01/02/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// General includes
#include <iostream>
#include <vector>
#include <iterator>

// CAN include
#include "jcan.h"


// Specifies the command used
enum BLCMDSendCommand {
    STOP                = 0x0,    // Disables all current through the motor, free spinning.
    FORWARD             = 0x1,    // Drive with FOC velocity control forward for 0.5s
    REVERSE             = 0x2,    // Drive with FOC velocity control forward for 0.5s
    DRIVE_VELOCITY      = 0x3,    // Drive with FOC velocity control at given signed integer speed
    DRIVE_POSITION      = 0x4,    // Drive with FOC position control to given angle. (-32,768 to +32,767) → (-π,π)
    DRIVE_CURRENT       = 0x5,    // Drive with FOC at selected current (torque)
    DRIVE_OPEN_LOOP     = 0x6,    // Drive open loop interpolating some set speed.
    HOME_ROTOR          = 0x7,    // Send request to home rotor
    ZERO_RESOLVER       = 0x8,    // Send request to zero resolver
    GET_CONFIG          = 0x9,    // Send request to get configuration
    SET_CONFIG          = 0xA     // Send request to set configuration
};

enum TelemetryPacket{
    PACKET_1 = 0x1,
    PACKET_2,
    PACKET_3,
    PACKET_4
};

enum CanIdPrefix{
    SEND = 0,
    RECEIVE = 4
};

enum BLCMDRecieveCommand{
    ERR_WARN_INF = 0x0,
    CONFIG_DATA = 0x8,
    WRITE_CONFIRMATION = 0x9
};

// Struct for Telemetry data
struct BLCMDTelemetry {
    double rotor_velocity;
    double q_current;
    double rotor_interval;
    double d_current;
    double resolver_position;
    double resolver_velocity;
    double power;
    double voltage;
    double temp;

};

std::ostream& operator << (std::ostream& o, BLCMDTelemetry& tel);

// Struct for Config set and get
enum ConfigVar {
    HAS_RESOLVER = 0,
    KP_CURRENT_LOOP,
    KI_CURRENT_LOOP,
    MAX_CURRENT_THRESHOLD,
    KP_VELOCITY_LOOP,
    KI_VELOCITY_LOOP,
    MAX_VELOCITY_THRESHOLD,
    MIN_INTERVAL,
    KP_POSITION_LOOP,
    KI_POSITION_LOOP,
    MAX_POSITION_THRESHOLD,
    TELEMETRY_P1_SPEED,
    TELEMETRY_P2_SPEED,
    TELEMETRY_P3_SPEED,
    TELEMETRY_P4_SPEED,
};

// Struct for Config set and get
struct BLCMDConfig {
    bool has_resolver;
    int16_t kp_current_loop;
    int16_t ki_current_loop;
    int16_t max_current_threshold;
    int16_t kp_velocity_loop;
    int16_t ki_velocity_loop;
    int16_t max_velocity_threshold;
    uint16_t min_interval;
    int16_t kp_position_loop;
    int16_t ki_position_loop;
    int16_t max_position_threshold;
    uint16_t telemetry_p1_speed;
    uint16_t telemetry_p2_speed;
    uint16_t telemetry_p3_speed;
    uint16_t telemetry_p4_speed;
};

// Class for storing information about the BLCMD
class BLCMD {

    //------------------------------------------------------------//
    private:

    // Can bus to use
    std::string bus;

    // BLCMD ID. BLCMD responds to CAN IDs in the range ID << 4 to ID << 4 + F
    int id;

    // Drive mode. Set to PWM or PID
    BLCMDSendCommand drive_mode;


    // Store whether we need to flip the output direction
    // 0 for regular, 1 for flipped
    bool direction;

    // Stop mode. Set to STOP or PID (handbrake)
    BLCMDSendCommand stop_mode;

    // Scaling factor to convert angular velocity (rad/s) to BLCMD command
    double scaling_factor;

    // Was the last command a STOP? If so, do not bother repeating
    bool already_stopped;
    
    // CAN bus for the CAN connection
    org::jcan::Bus *can_bus;

    public:
    /// @brief      Send a CAN frame with some command in the ID but no data
    /// @param      command - The command to send
    void write_frame_no_data (const BLCMDSendCommand command);

    /// @brief      Convert a double to an int16
    /// @param      value - The raw value between -1.0 and 1.0
    /// @returns    A Q15 fractional representing the same value
    static int16_t convert_to_int16 (const double value);

    /// @brief      Convert a 2-byte array to an int16_t
    /// @param      bytes - The 2-byte array
    /// @returns    an int16_t
    static int16_t from_bytes (uint8_t *bytes);

    /// @brief      Convert a 2-byte array to a double between 1 and -1.
    /// @param      bytes - The 2-byte array
    /// @returns    a double
    static double int16_bytes_to_double (uint8_t *bytes);

    /// @brief      Convert a 2-byte array to a double between 0 and 1.
    /// @param      bytes - The 2-byte array
    /// @returns    an double
    static double uint16_bytes_to_double (uint8_t *bytes);

    uint16_t make_can_id(BLCMDSendCommand command);

    uint16_t make_can_id(BLCMDRecieveCommand command);

    uint16_t make_can_id(TelemetryPacket packet);

    //------------------------------------------------------------//
//    public:

    // Maximum input to drive that will not saturate the CMD. Set by the scaling factor
    double max_speed;
    // Minimum positive speed that can be represented. Set by the max speed and 16-bit precision.
    double min_speed;

    /// @brief      Constructor for setting up a BLCMD interface
    /// @param      bus - The can bus to be used. 0 = "can0", 1 = "can1", 2 = "vcan0"
    /// @param      id - The ID of the CAN device on the CAN line
    /// @param      drive_mode - Default drive mode of the CMD. PWM or PID
    /// @param      direction - Direction for the CMD. Determined by hardware
    /// @param      stop_mode - Default stop mode of the CMD. STOP or PID (handbrake)
    /// @param      scaling_factor - Factor to multiply by input to convert from angular velocity (rad/s) to BLCMD command (unitless)
    BLCMD (const std::string bus, const int id, BLCMDSendCommand drive_mode, const bool direction=0, BLCMDSendCommand stop_mode=DRIVE_VELOCITY, double scaling_factor=1);

    /// @brief      Destructor is called when object is deleted
    ~BLCMD ();

    /// @brief      Calculate scaling factor to get CMD command from angular velocity
    /// @param      reduction - Gearbox reduction (input speed / output speed)
    /// @param      ppr - Encoder pulses per revolution (number of rising edges on one channel per revolution)
    /// @param      velocity_factor - BLCMD velocity factor. Used to scale the measureed velocity from the encoders on the BLCMD to fill the available int16 range
    /// @param      clock_frequency - BLCMD clock instruction frequency (FCY), measured in Hz
    /// @returns    The scaling factor in 1/(rad/s)
    static double get_scaling_factor(double reduction, int ppr, double velocity_factor, double clock_frequency);
    
    /// @brief      Get the BLCMD ID
    ///             Each BLCMD responds to CAN IDs in the range ID << 4 to ID << 4 + F
    /// @returns    The BLCMD ID
    int get_id();

    /// @brief      Set the BLCMD drive mode
    /// @param      drive_mode - Set to DRIVE_VELOCITY, DRIVE_POSITION,
    void set_drive_mode (BLCMDSendCommand drive_mode);

    /// @brief      Set the BLCMD stop mode
    /// @param      stop_mode - Set to STOP or PID
    void set_stop_mode (BLCMDSendCommand stop_mode);
    
    /// @brief      Send a CAN message to stop driving the CMD
    void stop ();

    /// @brief      Send a CAN message to drive forward at full speed
    void forward ();

    /// @brief      Send a CAN message to drive reverse at full speed
    void reverse ();

    /// @brief      Send a CAN message to drive the motor at the given velocity
    //TODO: expand on value param description
    /// @param      value - Value to send in drive command. Depends on Drive Mode
    void drive (float value);

    ///@brief       Send request to home the rotor
    ///@returns     a boolean for success or failure to home the rotor.
    bool home_rotor();

    ///@brief       Send request to zero resolver
    ///@returns     a boolean for success or failure to zero the resolver.
    bool zero_resolver();

    /// @brief      Gets a packet of telemetry and fills the relevant fields of a telemetry struct.
    /// @param      packet_num - the telemetry packet to get.
    /// @param      telemetry - pointer to a telemetry struct to get filled.
    void get_telemetry_packet (TelemetryPacket packet_num, BLCMDTelemetry* telemetry);

    /// @brief      Gets all packets of telemetry.
    /// @param      packet_num - the telemetry packet to get.
    /// @returns    A struct containing the data
    BLCMDTelemetry get_telemetry ();

    /// @brief      Gets a signle config variable from the blcmd.
    /// @param      var - the variable to get.
    /// @param      telemetry - pointer to a Config struct to get filled.
    void get_config_variable (ConfigVar var, BLCMDConfig* config);

    /// @brief      Gets all config data and returns a config struct.
    /// @returns    A struct containing the data.
    BLCMDConfig get_configuration ();

    /// @brief      Sets a single config variable.
    /// @param      var - The variable to be set.
    /// @param      value - The value to set the variable to in an int16_t format.
    /// @returns    a boolean for success or failure to set.
    bool set_config_variable (ConfigVar var, int16_t value);

    /// @brief      Sets all config variables.
    /// @param      config - The struct containing the config data to be set.
    /// @returns    a boolean for success or failure to set. Returns false if any are not set.
    bool set_config (BLCMDConfig *config);
};
