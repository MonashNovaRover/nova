/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "drive_publisher.h"

// Include standard output messages
#include <iostream>


// Adjustes the multiplier factor by some amount in some direction
float DrivePublisher::adjust_multiplier (float& multiplier, bool increase) {

    // Adjust the multiplier
    multiplier += (increase) ? DELTA_MULTIPLIER : -DELTA_MULTIPLIER;

    // Check for minimum and maximums
    if (multiplier > MAX_MULTIPLIER)
        multiplier = MAX_MULTIPLIER;
    else if (multiplier <= MIN_MULTIPLIER)
        multiplier = MIN_MULTIPLIER;

    // Return the new multiplier
    return multiplier;
}


// Publishes the drive commands from the speed and steer
void DrivePublisher::publish_cmds () {

    // Create the message
    auto message = core::msg::DriveCmd();

    // Set up the values if the controller is not locked
    if (!locked && connected) {
        message.speed = input_axis_y * multiplier_speed * trigger_speed;
        message.steer = input_axis_x * multiplier_steer;
    
    // Otherwise print lock message
    } else if (locked) {
        cout << "Controller LOCKED." << endl;
        fflush(stdout);
    }
    
    // Publish the drive commands
    publisher->publish(message);
}


// Receives input from the gamepad
void DrivePublisher::input_callback (const core::msg::InputGamepad::SharedPtr msg) {

    // Get the connection state
    connected = msg->connected;

    // If no connection, reset the state
    if (!msg->connected) {
        input_axis_x = 0.0;
        input_axis_y = 0.0;
        trigger_speed = 1.0;

        // Publish no connection message
        cout << "No Controller Connected." << endl;
        fflush(stdout);
    }

    // If the controller is connected
    else {
        // Update the input axis
        input_axis_x = msg->ax_stick_r_x;
        input_axis_y = msg->ax_stick_l_y;

        // Get the trigger speed multiplier
        trigger_speed = 1.0 - (msg->trg_r_val * (1 - MIN_TRIGGER_MULTIPLIER));

        // Determine if the conrroller needs to be locked or not
        if (msg->btn_back_state > 0)
            locked = true;
        if (msg->btn_start_state > 0)
            locked = false;
        

        // Prevent changing states if the controller is locked
        if (locked)
            return;

        // Change the speed multipliers
        if (msg->btn_dpad_u_state == 1)
            adjust_multiplier(multiplier_speed, true);
        else if (msg->btn_dpad_d_state == 1)
            adjust_multiplier(multiplier_speed, false);
        
        // Change the steer multipliers
        if (msg->btn_dpad_r_state == 1)
            adjust_multiplier(multiplier_steer, true);
        else if (msg->btn_dpad_l_state == 1)
            adjust_multiplier(multiplier_steer, false);
    }         
}


// Main constructor that sets up the node
DrivePublisher::DrivePublisher() 
  : Node("drive_pub"), count(0) {

    // Creates the publisher
    publisher = this->create_publisher<core::msg::DriveCmd>("/control/drive_cmds", 10);
    
    // Creates the input subscription
    subscription = this->create_subscription<core::msg::InputGamepad>(
        "/control/input_gamepad", 10, std::bind(&DrivePublisher::input_callback, this, _1));

    // Creates a timer function that runs a function on loop every 0.05 seconds
    timer = this->create_wall_timer(50ms, std::bind(&DrivePublisher::publish_cmds, this));
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