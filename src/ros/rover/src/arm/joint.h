#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This code interfaces with the CMD class that is able
    to send CAN messages. 

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Jess Hepworth
CREATION:	04/12/2021
EDITED:		04/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - needs a function to set drive mode
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// General includes
#include "../cmd/cmd.h"


// Joint class for communicating with joint CMDs
class Joint : public CMD {

    //------------------------------------------------------------//
    public:

    // Stores the drive mode for the joint's CMD
    // 0 for PWM, 1 for PID
    CMDCommand CMD_drive_mode;

    //------------------------------------------------------------//
    protected:

    /// @brief      Destructor is called when object is deleted
    ~Joint ();

    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor for setting up a joint
    /// @param      id - The new ID for the joint for CAN commands
    /// @param      CMD_drive_mode - Default drive mode of the joint CMD 
    Joint (const int id, CMDCommand CMD_drive_mode);

    /// @brief      Drives the joint based on a speed
    /// @param      velocity - The speed to move, between -1 and 1
    void drive (float velocity);
    
    /// @brief      Sends ALL STOPS commands to the joints
    void stop () override;

    /// @brief      sets the CMD drive mode
    /// @param      CMD_drive_mode - Drive mode to set for the joint CMD 
    void set_CMD_drive_mode (CMDCommand CMD_drive_mode);
};
