#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class implements a generic PI controller
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: None
TOPICS: None
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	   control
AUTHOR(S):     Jory Braun
CREATION:	   02/10/2022
EDITED:		   02/10/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/


class PIController
{
    //------------------------------------------------------------//
    private:

    // Store the control gains
    const double prop_gain;
    const double int_gain;

    // Track the internal state
    double prev_error;
    double integral;


    //------------------------------------------------------------//
    public:

    // Track saturation state. If set, will apply to following calls to update
    bool saturated;
    
    
    /// @brief  Constructor. Initialises the controller with the given control coefficients
    PIController(const double prop_gain, const double int_gain);
    
    /// @brief  Update the controller, get the new output 
    double update(double error, double timestep);

};
