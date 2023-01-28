/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/


#include "pi_controller.h"


PIController::PIController(const double prop_gain, const double int_gain) :
    prop_gain(prop_gain),
    int_gain(int_gain),
    // Initialise in zero-state
    prev_error(0),
    integral(0),
    saturated(false)
{

}


double PIController::update(double error, double timestep)
{
    // Update the integral term. use trapezoidal integration
    integral += saturated ? 0 : (error + prev_error) / 2 * timestep;
    prev_error = error;
    // Get the output
    return int_gain * error + int_gain * integral;
}
