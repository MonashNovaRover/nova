#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This code interfaces with the CAN classes and is
    able to communicate with all of the BLCMD/PACMANs
    electronic code by creating instances of each class.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Harrison Verrios, Josh Cherubino, Jory Braun, Taaj Street
CREATION:	01/12/2021
EDITED:		13/09/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// General includes
#include <iostream>
#include <vector>
#include <iterator>

// CAN include
#include "jcan/jcan.h"


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
    PACKET_2 = 0x2,
    PACKET_3 = 0x3,
    PACKET_4 = 0x4
};

// TODO: Set for human readable values
// Struct for Telemetry data
struct BLCMDTelemetry {
    int16_t rotor_velocity;
    int16_t q_current;
    uint16_t rotor_interval;
    int16_t d_current;
    int16_t resolver_position;
    int16_t resolver_velocity;
    uint16_t power;
    uint16_t voltage;
    uint16_t temp;

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

    // CAN bus ID (0 or 1)
    int bus;

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
    static int16_t bytes_to_int16 (uint8_t *bytes);
    
    //------------------------------------------------------------//
    public:

    // Maximum input to drive that will not saturate the CMD. Set by the scaling factor
    double max_speed;
    // Minimum positive speed that can be represented. Set by the max speed and 16-bit precision.
    double min_speed;

    /// @brief      Constructor for setting up a BLCMD interface
    /// @param      bus - The bus ID of the CAN device
    /// @param      id - The ID of the CAN device on the CAN line
    /// @param      drive_mode - Default drive mode of the CMD. PWM or PID
    /// @param      direction - Direction for the CMD. Determined by hardware
    /// @param      stop_mode - Default stop mode of the CMD. STOP or PID (handbrake)
    /// @param      scaling_factor - Factor to multiply by input to convert from angular velocity (rad/s) to BLCMD command (unitless)
    BLCMD (const int bus, const int id, BLCMDSendCommand drive_mode, const bool direction=0, BLCMDSendCommand stop_mode=STOP, double scaling_factor=1);

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
    /// @param      drive_mode - Set to PWM or PID
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

    /// @brief      Function for sending linear actuator command to CMD
    /// @param      value - number from thumb stick (-1, 0, 1)
    void set_linear_actuator (float value);
    
    /// @brief      Sends PID commands to the device
    /// @param      kP - The Proportionality constant
    /// @param      kI - The Intergral constant
    /// @param      kD - The Differential constant
    /// @param      kM - The Midpoint interval
    void set_tuning_parameters (double kP, double kI, double kD, double kM);

    //TODO: Better way to return packets of telemetry.
    /// @brief      Gets a packet of telemetry and returns a partially filled BLCMDTelemetry struct.
    /// @returns    A struct containing the data
    BLCMDTelemetry get_telemetry_packet (TelemetryPacket packet_num);



};
