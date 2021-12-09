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
EDITED:		01/12/2021
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
    

};
