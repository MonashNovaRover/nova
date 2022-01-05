/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "pid_tuner.h"
#include "debug/print.h"


// Tunes the PID controls
void PIDTuner::select_device (
    const core::srv::PIDTune::Request::SharedPtr request,
        core::srv::PIDTune::Response::SharedPtr response) 
{

    // Check for invalid bus
    if (request->bus < 0 || request->bus > 1) {
        response->success = false;
        return;
    }

    // Check for invalid wheel
    if (request->bus == 0 && (request->id < 1 || request->id > NUM_WHEELS)) {
        response->success = false;
        return;
    }

    // Check for invalid arm
    if (request->bus == 1 && (request->id < 1 || request->id > NUM_ARM_DEVICES)) {
        response->success = false;
        return;
    }

    // Update the bus and id selected
    this->bus = request->bus;
    this->id = request->id;
    this->valid = true;

    // Check for invalid constants
    if (request->p == 0 || request->i == 0 || request->d == 0) {
        response->success = false;
        return;
    }

    // Return a success
    response->success = true;

}


// Constructor initialises all the devices
PIDTuner::PIDTuner ()
  : Node("pid_tuner"), count(0) {

    // Construct the array of wheels
    for (int i = 0; i < NUM_WHEELS; i++) {
        bus_0[i] = new CMD(0, i + 1);
    }

    // Construct the array of arm devices
    for (int i = 0; i < NUM_ARM_DEVICES; i++) {
        bus_1[i] = new CMD(1, i + 1);
    }

    // Create the service and bind to the function
    service = this->create_service<core::srv::PIDTune>("/control/pid_tune", 
        std::bind(&PIDTuner::select_device, this, _1, _2));

}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<PIDTuner>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}