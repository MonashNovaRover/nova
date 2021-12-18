/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE:  control
AUTHOR(S):  Himsara Gallege
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "vel_convert.h"

// Receives velocity converters
void DriveVelocity::velocity_callback (const core::msg::DriveVel::SharedPtr msg) {
    
    // Create the message
    auto message = core::msg::DriveInput();

    message.speed = .5 * msg->linear_vel / (CONVERSION_FACTOR / 60.0 * 0.785398 * 0.95) / 100.0;
    message.steer = .5 * msg->angular_vel * STEER_FACTOR;

    // Publish the drive commands
    publisher->publish(message);

}

// Main constructor that sets up the node
DriveVelocity::DriveVelocity() 
  : Node("drive_velocity"), count(0) {

    // Creates the publisher
    publisher = this->create_publisher<core::msg::DriveInput>("/autonomous/drive_inputs", 10);
    
    // Creates the input subscription
    subscription = this->create_subscription<core::msg::DriveVel>(
        "/autonomous/drive_velocity", 10, std::bind(&DriveVelocity::velocity_callback, this, _1));

}

//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<DriveVelocity>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}