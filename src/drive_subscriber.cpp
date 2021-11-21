/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "drive_subscriber.h"

// Include standard output messages
#include <iostream>

// Receives drive commands
void DriveSubscriber::drive_callback (const core::msg::DriveCmd::SharedPtr msg) {
    cout << msg->speed << " " << msg->steer << endl;
    fflush(stdout);
}

// Receives input from the gamepad
void DriveSubscriber::input_callback (const core::msg::InputGamepad::SharedPtr msg) {

}


// Main constructor that sets up the node
DriveSubscriber::DriveSubscriber() 
  : Node("drive_sub"), count(0) {

    // Creates the commands subscription
    subscription_cmds = this->create_subscription<core::msg::DriveCmd>(
        "/control/drive_cmds", 10, std::bind(&DriveSubscriber::drive_callback, this, _1));
    
    // Creates the input subscription
    subscription_inputs = this->create_subscription<core::msg::InputGamepad>(
        "/control/input_gamepad", 10, std::bind(&DriveSubscriber::input_callback, this, _1));
}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Subscriber class
    rclcpp::spin(std::make_shared<DriveSubscriber>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}