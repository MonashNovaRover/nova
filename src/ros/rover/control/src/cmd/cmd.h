#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This code interfaces with the CAN classes and is
    able to communicate with all of the CMDs
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

// CAN include
#include "jcan/jcan.h"


// Specifies the command used
enum CMDCommand {
    STOP           = 0,    // All stops commands
    FORWARD        = 1,    // Drives forward at full power
    REVERSE        = 2,    // Drives reverse at full power
    PWM            = 3,    // Drives with PWM control
    PID            = 4,    // Drives with PID control
    GET_TUNE       = 5,    // Get PID tuning constants
    SET_TUNE       = 6,    // Set PID tuning constants
    ACTUATOR       = 7,    // Control the linear actuator
    FAST_TELEMETRY = 8     // Swap telemetry between fast and slow publishing speeds
};


// Struct for CMD data
struct CMDData {

    // The velocity from the CMD
    double rpm;

    // The power of the CMD
    double power;

    // Constructor for setting the data
    CMDData (double rpm, double power) : 
        rpm(rpm), power(power) {}
};


// Class for storing information about the CMD
class CMD {

    //------------------------------------------------------------//
    private:

    // CAN bus ID (0 or 1)
    int bus;

    // CMD ID. CMD responds to CAN IDs in the range ID << 4 to ID << 4 + F
    int id;

    // Drive mode. Set to PWM or PID
    CMDCommand drive_mode;

    // Store whether we need to flip the output direction
    // 0 for regular, 1 for flipped
    bool direction;

    // Stop mode. Set to STOP or PID (handbrake)
    CMDCommand stop_mode;

    // Scaling factor to convert angular velocity (rad/s) to CMD command
    double scaling_factor;

    // Was the last command a STOP? If so, do not bother repeating
    bool already_stopped;
    
    // CAN bus for the CAN connection
    org::jcan::Bus *can_bus;


    /// @brief      Send a CAN frmae with some command in the ID but no data
    /// @param      command - The command to send
    void write_frame_no_data (const CMDCommand command);

    /// @brief      Convert a double to an int16
    /// @param      value - The raw value between -1.0 and 1.0
    /// @returns    A Q15 fractional representing the same value
    static int16_t convert_to_int16 (const double value);

    /// @brief      Convert a 2-byte array to a double
    /// @param      bytes - The 2-byte array
    /// @returns    A double scaled between -1 and 1
    static double convert_from_bytes (uint8_t* bytes);
    
    //------------------------------------------------------------//
    public:

    // Maximum input to drive that will not saturate the CMD. Set by the scaling factor
    double max_speed;
    // Minimum positive speed that can be represented. Set by the max speed and 16-bit precision.
    double min_speed;

    /// @brief      Constructor for setting up a CMD interface
    /// @param      bus - The bus ID of the CAN device
    /// @param      id - The ID of the CAN device on the CAN line
    /// @param      drive_mode - Default drive mode of the CMD. PWM or PID
    /// @param      direction - Direction for the CMD. Determined by hardware
    /// @param      stop_mode - Default stop mode of the CMD. STOP or PID (handbrake)
    /// @param      scaling_factor - Factor to multiply by input to convert from angular velocity (rad/s) to CMD command (unitless)
    CMD (const int bus, const int id, CMDCommand drive_mode, const bool direction=0, CMDCommand stop_mode=STOP, double scaling_factor=1);

    /// @brief      Destructor is called when object is deleted
    ~CMD ();

    /// @brief      Calculate scaling factor to get CMD command from angular velocity
    /// @param      reduction - Gearbox reduction (input speed / output speed)
    /// @param      ppr - Encoder pulses per revolution (number of rising edges on one channel per revolution)
    /// @param      velocity_factor - CMD velocity factor. Used to scale the measureed velocity from the encoders on the CMD to fill the available int16 range
    /// @param      clock_frequency - CMD clock instruction frequency (FCY), measured in Hz
    /// @returns    The scaling factor in 1/(rad/s)
    static double get_scaling_factor(double reduction, int ppr, double velocity_factor, double clock_frequency);
    
    /// @brief      Get the CMD ID
    ///             Each CMD responds to CAN IDs in the range ID << 4 to ID << 4 + F
    /// @returns    The CMD ID
    int get_id();

    /// @brief      Set the CMD drive mode
    /// @param      drive_mode - Set to PWM or PID
    void set_drive_mode (CMDCommand drive_mode);

    /// @brief      Set the CMD stop mode
    /// @param      stop_mode - Set to STOP or PID
    void set_stop_mode (CMDCommand stop_mode);
    
    /// @brief      Send a CAN message to stop driving the CMD
    void stop ();

    /// @brief      Send a CAN message to drive forward at full speed
    void forward ();

    /// @brief      Send a CAN message to drive reverse at full speed
    void reverse ();

    /// @brief      Send a CAN message to drive the motor at the given velocity
    /// @param      velocity - Motor velocity. If the scaling factor set, then is
    ///             in rad/s. Otherwise is a fraction of the max CMD speed between -1 and 1
    void drive (float velocity);

    /// @brief      Function for sending linear actuator command to CMD
    /// @param      value - number from thumb stick (-1, 0, 1)
    void set_linear_actuator (float value);
    
    /// @brief      Sends PID commands to the device
    /// @param      kP - The Proportionality constant
    /// @param      kI - The Intergral constant
    /// @param      kD - The Differential constant
    /// @param      kM - The Midpoint interval
    void set_tuning_parameters (double kP, double kI, double kD, double kM);

    /// @brief      Receives feedback from the CMD devices on the CAN lines
    /// @returns    A struct containing the data
    CMDData receive_feedback ();

};
