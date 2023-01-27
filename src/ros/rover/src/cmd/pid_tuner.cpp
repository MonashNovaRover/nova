/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "pid_tuner.h"
#include "print/print.h"


// Selects the device
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
    this->velocity = request->velocity;
    this->valid = true;

    // Check for invalid constants
    if (request->p == 0 || request->i == 0 || request->d == 0) {
        response->success = false;
        return;
    }

    // Check for out of range constants
    if (request->p > 1.0 || request->i > 1.0 || request->d > 1.0 || request-> m > 1.0) {
        response->success = false;
        return;
    }

    // Updates the constants on the device
    update_constants(request->p, request->i, request->d, request->m);

    // Return a success
    response->success = true;

}


// Updates the constanst
void PIDTuner::update_constants (double kP, double kI, double kD, double kM) {

    // Make sure a valid ID is set
    if (!this->valid) return;

    // Get the current selected CMD
    CMD* cmd = this->get_cmd();

    // Run the constants
    cmd->set_tuning_parameters(kP, kI, kD, kM);
}


// Return the current valid CMD
CMD* PIDTuner::get_cmd () {

    // Check for each bus
    if (bus == 0) return bus_0[id - 1];
    if (bus == 1) return bus_1[id - 1];

    // Invalid bus
    return nullptr;
}


// Sends the velocities
void PIDTuner::send_velocity () {
    
    // Make sure it is valid
    if (!this->valid) return;

    // Send PID velocity data to the CMD
    CMD* cmd = this->get_cmd();

    // Send the velocity to the CMD
    cmd->drive(velocity);
}


// Publishes the feedback
void PIDTuner::publish_velocity () {

    // Check for invalid id
    if (!valid) return;

    // Construct the feedback message
    auto message = core::msg::CMDFeedback();

    // Update the IDs
    message.bus = this->bus;
    message.id = this->id;

    // Get the data
    CMDData data = this->get_cmd()->receive_feedback();

    // Get the data
    message.omega = data.rpm;
    message.duty_cycle = data.power;

    // Make sure data is valid
    if (data.rpm != 0 || data.power != 0) {

        // Publish the data
        publisher->publish(message);
    }
}


// Constructor initialises all the devices
PIDTuner::PIDTuner () : Node("pid_tuner")
{

    // Create the service and bind to the function
    service = this->create_service<core::srv::PIDTune>(
        "/control/pid_tune", std::bind(&PIDTuner::select_device, this, _1, _2)
    );

    // Creates the publisher
    publisher = this->create_publisher<core::msg::CMDFeedback>("/control/cmd_feedback", 10);

    // Creates a timer function that runs a function on loop every 0.1 seconds
    velocity_timer = this->create_wall_timer(100ms, std::bind(&PIDTuner::send_velocity, this));

    // Creates a timer function that runs a function on loop every 0.05 seconds
    feedback_timer = this->create_wall_timer(50ms, std::bind(&PIDTuner::publish_velocity, this));

    // Output set-up messages
    Print::title("PID TUNER");
    Print::print("Valid Topics:");
    Print::print("/control/cmd_feedback         [CMDFeedback]", 1);
    Print::print("", true);
    Print::print("Valid Services:");
    Print::print("/control/pid_tune             [PIDTune]", 1);
    Print::print("", true);

    // Construct the array of wheels
    for (int i = 0; i < NUM_WHEELS; i++) {
        bus_0[i] = new CMD(0, i + 1, PID, PID);
    }

    // Construct the array of arm devices
    for (int i = 0; i < NUM_ARM_DEVICES; i++) {
        bus_1[i] = new CMD(1, i + 1, PID, PID);
    }
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
