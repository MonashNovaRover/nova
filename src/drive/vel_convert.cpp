/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE:  control
AUTHOR(S):  Himsara Gallege
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "vel_convert.h"

// Include standard output messages
#include <iostream>


// Publishes the velocity
void VelConvert::publish_cmds () {
    auto message = core::msg::DriveCmd();

    message.speed = .5 * data.linear_vel / (self.conversion_factor / 60.0 * 0.785398 * 0.95) 
    message.steer = .5 * data.angular_vel * self.steer_factor * 100
    
    // Publish the drive commands
    publisher->publish(message);
}
















// Receives input from the gamepad
void DrivePublisher::input_callback (const core::msg::InputGamepad::SharedPtr msg) {

    linear_vel = msg->linear_vel;
    angular_vel = msg->angular_vel;










//----------------------------------------------------------------------------------------------------

// Main constructor that sets up the node
DrivePublisher::DrivePublisher() 
  : Node("vel_pub"), count(0) {

    // Creates the publisher
    publisher = this->create_publisher<core::msg::DriveCmd>("/control/drive_cmds", 10);
    
    // Creates the input subscription
    subscription = this->create_subscription<core::msg::DriveVel>(
        "/control/drive_vel", 10, std::bind(&DrivePublisher::input_callback, this, _1));
}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<DrivePublisher>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}