/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE:    control
AUTHOR(S):  Himsara Gallege, Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "velocity_convert.h"
#include "print/print.h"
#include <stdio.h>

// Use the standard namespaces for subscribers
using std::placeholders::_1;

// Create any definitions
#define PI      3.141593    // Mathematical constant PI
#define S_PER_M 60          // Seconds per Minute


// Receives velocity commands
void VelocityConvert::velocity_callback (const core::msg::DriveVel::SharedPtr msg) {
    
    // Create the message
    auto message = core::msg::DriveInput();

    // TODO
    // Calculate the RPM value
    //const float RPM = msg->linear_vel * S_PER_M / (2 * PI * WHEEL_RADIUS);

    // Calculate the new message values
    message.speed = msg->linear_vel / MAX_SPEED;
    message.steer = msg->angular_vel * STEER_FACTOR;

    std::cout << message.speed << "speed" << message.steer;
    std::fflush(stdout);
    //Print::print(message, C_MODE);

    // Publish the drive commands
    publisher->publish(message);

}


// Main constructor that sets up the node
VelocityConvert::VelocityConvert() : Node("velocity_convert")
{
    // Creates the publisher
    publisher = this->create_publisher<core::msg::DriveInput>("/autonomous/drive_inputs", 10);
    
    // Creates the input subscription
    this->create_subscription<core::msg::DriveVel>(
        "/autonomous/drive_velocity", 10, std::bind(&VelocityConvert::velocity_callback, this, _1));

}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    Print::print("tester", C_MODE);
    //ROS_DEBUG("test2");
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<VelocityConvert>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}
