/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jess Hepworth
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "arm_driver.h"
#include "debug/print.h"

/*
// Receives the desired commands for the CMDs and sends to CMDs
void ArmDriver::cmd_outputs_callback (const core::msg::CMDOutput::SharedPtr msg) {

    for (auto i = 0; i < NUM_JOINTS; i++) {
        joints[i]->drive(msg->velocity[i]) //TODO: fix this after message is written
    }
}
*/


// Receives the desired commands for the CMDs and sends to CMDs
void ArmDriver::arm_input_callback (const core::msg::ArmInput::SharedPtr msg) {

    // Receiving data for first 6 joints
    for (auto i = 0; i < NUM_JOINTS; i++) {
        joints[i]->drive(msg->joint_velocity[i]);
    }

    // Receiving data for end effector
    joints[6]->drive(msg->end_effector_actuation); //need to create message

    // Linear actuator
    joints[6]->set_linear_actuator(msg->linear_actuation);
}


// Main constructor that sets up the node
ArmDriver::ArmDriver() 
  : Node("arm_driver"), count(0) {
    
    // Create joint instances 
    for (int i = 0; i < (NUM_JOINTS + 1); i++) {
        joints[i] = new Joint (i + 1, CMD_drive_mode[i]);
    }

    /*
    // Creates the input subscription for the desired CMD commands
    cmd_outputs_subscription = this->create_subscription<core::msg::CMDOutput>(
        "/control/cmd_outputs", 10, std::bind(&ArmDriver::cmd_outputs_callback, this, _1));    
    */

    // Creates the input subscription for the desired CMD commands
    arm_input_subscription = this->create_subscription<core::msg::ArmInput>(
        "/control/arm_input", 10, std::bind(&ArmDriver::arm_input_callback, this, _1));

    // Output title messages
    Print::title("ARM DRIVER");
    Print::print("", true);

}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ArmDriver>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}