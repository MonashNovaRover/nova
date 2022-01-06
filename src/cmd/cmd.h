#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This code interfaces with the CAN classes and is
    able to communicate with all of the CMDs
    electronic code by creating instances of each class.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Harrison Verrios, Josh Cherubino
CREATION:	01/12/2021
EDITED:		06/01/2022
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

    //------------------------------------------------------------//
    public:

    // The RPM from the CMD
    double rpm;

    // The power of the CMD
    double power;

    // Constructor for setting the data
    CMDData (double rpm, double power) {
        this->rpm = rpm;
        this->power = power;
    };
};


// Class for storing information about the CMD
class CMD {

    //------------------------------------------------------------//
    protected:

    // The bus ID for the CMD (0 or 1)
    int bus;

    // The identification ID for the CMD
    int id;
    
    // The CAN socket that gets initialised for the CAN connection
    scpp::SocketCan can_socket;


    //------------------------------------------------------------//
    protected:

    /// @brief      Calls an empty data frame packet for some command
    /// @param      command - The command to send
    void call_empty (const CMDCommand command);

    /// @brief      Destructor is called when object is deleted
    ~CMD ();


    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor for setting up a CMD interface
    /// @param      bus - The bus ID of the CAN device
    /// @param      id - The ID of the CAN device on the CAN line
    CMD (const int bus, const int id);

    /// @brief      Stops all speeds on this particular CMD
    virtual void stop ();

    /// @brief      Drives forward at full speed to the CMD
    void forward ();

    /// @brief      Drives reverse at full speed to the CMD
    void reverse ();

    /// @brief      Sends PWM commands to the CMD on the CAN line
    /// @param      power - The fraction between -1.0 and 1.0 to send
    void set_pwm (float power);

    /// @brief      Sends PID commands to the CMD on the CAN line
    /// @param      speed - The fraction between -1.0 and 1.0 to send
    void set_pid (float speed);

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


    //------------------------------------------------------------//

    /// @brief      Converts a double to an int16
    /// @param      value - The raw value between -1.0 and 1.0
    /// @returns    A two byte array of integers
    static int16_t convert_to_int16 (const double value);

    /// @brief      Converts a 2 byte array to a double
    /// @param      bytes - The two byte array
    /// @returns    A double scaled between -1 and 1
    static double convert_from_bytes (uint8_t* bytes);
};
