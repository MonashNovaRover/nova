#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This code interfaces with the CMD class that is able
    to send CAN messages. It can send wheel data from
    each of the wheels with the correct PID or PWM
    speeds.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Harrison Verrios, Josh Cherubino
CREATION:	21/11/2021
EDITED:		09/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// General includes
#include "cmd/cmd.h"


// Wheel class for communicating with wheel CMDs
class Wheel : public CMD {

    //------------------------------------------------------------//
    protected:

    // Whether this wheel is on the left or right of the rover
    bool left;


    //------------------------------------------------------------//
    protected:

    /// @brief      Destructor is called when object is deleted
    ~Wheel ();


    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor for setting up the wheel
    /// @param      id - The new ID for the wheel for CAN commands
    /// @param      left - A flag for whether the wheel is on the left
    Wheel (const int id, const bool left);

    /// @brief      Spins the wheel based on a speed
    /// @param      speed - The speed to move, between -1 and 1
    void spin (float speed);

    /// @brief      Moves the wheel and communicates to the classes based on some data
    /// @param      speed - The speed to move, between -1 and 1
    /// @param      steer - The steer where + is right and - is left
    void spin (float speed, const float steer);
    
    /// @brief      Sends ALL STOPS commands to the wheels
    void stop () override;

    /// @brief      Returns the ID of the wheel
    /// @returns    wheel ID
    int get_id ();
};
