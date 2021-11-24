#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This code interfaces with the CAN classes and is
    able to communicate with all of the wheel CMDs
    by creating instances of each class.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
CREATION:	21/11/2021
EDITED:		21/11/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// General includes
#include <iostream>

// CAN include
#include "socketcan/socketcan_cpp.h"

enum WheelCommand {
    STOP = 0,
    FORWARD_FULL,
    REVERSE_FULL,
    SET_PWM,
    SET_VELOCITY,
    PID_TUNE,
    SET_LINEAR_ACTUATOR //TODO: Remove because only for arm?
};

// Wheel class for communicating with wheel CMDs
class Wheel {

    //------------------------------------------------------------//
    protected:

    // The identification id for the wheel
    int id;

    // The direction that the wheel will turn with positive speed
    bool clockwise;
    
    scpp::SocketCan can_socket;

    //------------------------------------------------------------//
    protected:

    /// @brief      Destructor is called when object is deleted
    ~Wheel ();


    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor for setting up the wheel
    /// @param      id - The new ID for the wheel for CAN commands
    /// @param      clockwise - Direction the wheel will spin by default
    Wheel (const int id, const bool clockwise);

    /// @brief      Spins the wheel based on a speed
    /// @param      speed - The speed to move, between -1 and 1
    void spin (float speed);

    /// @brief      Moves the wheel and communicates to the classes based on some data
    /// @param      speed - The speed to move, between -1 and 1
    /// @param      steer - The steer where + is right and - is left
    void spin (float speed, const float steer);
    
};
