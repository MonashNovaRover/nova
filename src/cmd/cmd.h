#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This code interfaces with the CAN classes and is
    able to communicate with all of the CMDs
    electronic code by creating instances of each class.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Harrison Verrios, Josh Cherubino, Jory Braun
CREATION:	01/12/2021
EDITED:		13/09/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// General includes
#include <iostream>

// CAN include
#include "socketcan/socketcan_cpp.h"


// Specifies the command used
enum CMDCommand {
    STOP        = 0,    // All stops commands
    FORWARD     = 1,    // Drives forward at full power
    REVERSE     = 2,    // Drives reverse at full power
    PWM         = 3,    // Drives with PWM control
    PID         = 4,    // Drives with PID control
    TUNER       = 5,    // Sends PID tuning commands
    ACTUATOR    = 6,    // Controls the linear actuator
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


struct CMDOutputParameters {
    // Set to true if using output parameters
    bool using_output_parameters;

    // Gearbox reduction (input speed / output speed)
    float reduction;
    
    // Encoder pulses per revolution (number of rising edges on one channel per revolution)
    int ppr;
    
    // CMD velocity factor
    // Used to scale the measureed velocity from the encoders on the CMD to fill the available int16 range
    float velocity_factor;
    
    // CMD clock instruction frequency (FCY), measured in Hz
    float clock_frequency;

    // Scaling factor to get CMD command from angular velocity
    float command_scale_factor;

    // Constructor for setting the data
    CMDOutputParameters (float reduction, int ppr, float velocity_factor, float clock_frequency);

    // Alternate constructor for when not using output parameters
    CMDOutputParameters() : using_output_parameters(false) {}
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
    // Stop mode. Set to STOP or PID (handbrake)
    CMDCommand stop_mode;

    // Store whether we need to flip the output direction
    // 0 for regular, 1 for flipped
    bool direction;

    // Output parameters
    CMDOutputParameters output_parameters;

    // Was the last command a STOP? If so, do not bother repeating
    bool already_stopped;
    
    // CAN socket for the CAN connection
    scpp::SocketCan can_socket;


    /// @brief      Send a CAN frmae with some command in the ID but no data
    /// @param      command - The command to send
    void write_frame_no_data (const CMDCommand command);
    
    //------------------------------------------------------------//
    public:

    /// @brief      Constructor for setting up a CMD interface
    /// @param      bus - The bus ID of the CAN device
    /// @param      id - The ID of the CAN device on the CAN line
    /// @param      drive_mode - Default drive mode of the CMD. PWM or PID
    /// @param      stop_mode - Default stop mode of the CMD. STOP or PID (handbrake)
    /// @param      direction - Direction for the CMD. Determined by hardware
    /// @param      output_parameters - Parameters to ensure correct angular velocity is achieved by the CMD
    CMD (const int bus, const int id, CMDCommand drive_mode, CMDCommand stop_mode=STOP, const bool direction=0, CMDOutputParameters output_parameters=CMDOutputParameters());

    /// @brief      Destructor is called when object is deleted
    ~CMD ();

    /// @brief      Convert a double to an int16
    /// @param      value - The raw value between -1.0 and 1.0
    /// @returns    A Q15 fractional representing the same value
    static int16_t convert_to_int16 (const double value);

    /// @brief      Convert a 2-byte array to a double
    /// @param      bytes - The 2-byte array
    /// @returns    A double scaled between -1 and 1
    static double convert_from_bytes (uint8_t* bytes);
    
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
    /// @param      velocity - Motor velocity. If the output parameters are set, then is
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
