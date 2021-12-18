/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios, Josh Cherubino
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "driver.h"
#include "debug/print.h"

// Sends commands to the wheels
void Driver::send_commands (const core::msg::DriveInput::SharedPtr msg) {
    
    // Check if wheels should spin
    if (msg->speed != 0.0 || msg->steer != 0.0) {
        // Spin the wheels
        for (Wheel* wheel : wheels) {
            wheel->spin(msg->speed, msg->steer);
        }

        // Reset the zero flag
        stopped_sent = false;
    }

    // Otherwise, if handbrake is on, send zeros
    else if (handbrake) {
        // Spin the wheels for 0 speed
        for (Wheel* wheel : wheels) {
            wheel->spin(0.0);
        }
    }

    // Otherwise, if handbrake is not on, only send on lot of zero speeds
    else if (!stopped_sent) {
        // Spin the wheels for 0 speed
        for (Wheel* wheel : wheels) {
            wheel->stop();
        }

        // Set the stopped flag so it doesn't run again
        stopped_sent = true;
    }
}


// Receives drive commands
void Driver::drive_callback (const core::msg::DriveInput::SharedPtr msg) {

    // If manual driving state, call the commands
    if (!is_autonomous)
        send_commands(msg);    
}


// Receives autonomous commands
void Driver::auto_callback (const core::msg::DriveInput::SharedPtr msg) {

    // If autonomous driving state, call the commands
    if (is_autonomous)
        send_commands(msg); 
}


// Receives input from the gamepad
void Driver::input_callback (const core::msg::InputGamepad::SharedPtr msg) {

    // Enable or Disable handbraking based on the thumb buttons
    if (msg->connected && msg->btn_thumb_l_state == 1)
        handbrake = true;
    else if (msg->connected && msg->btn_thumb_r_state == 1)
        handbrake = false;

    // Enable or disable autonomous
    if (msg->connected && msg->btn_a_state == 1)
        is_autonomous = true;
    else if (msg->connected && msg->btn_b_state == 1)
        is_autonomous = false;
}


// Main constructor that sets up the node
Driver::Driver() 
  : Node("drive_sub"), count(0) {

    // Output set-up messages
    Print::title("DRIVER");
    Print::print("", true);

    // Initialise the wheels in the correct direction
    for (int i = 0; i < NUM_WHEELS; i++) {
        bool clockwise = i < NUM_WHEELS / 2;
        wheels[i] = new Wheel (i + 1, clockwise);
    }

    // TODO QoS Profiles

    // Creates the commands subscription (manual)
    subscription_cmds_man = this->create_subscription<core::msg::DriveInput>(
        "/control/drive_inputs", 10, std::bind(&Driver::drive_callback, this, _1));
    
    // Creates the commands subscription (autonomous)
    subscription_cmds_auto = this->create_subscription<core::msg::DriveInput>(
        "/autonomous/drive_inputs", 10, std::bind(&Driver::auto_callback, this, _1));
    
    // Creates the input subscription
    subscription_inputs = this->create_subscription<core::msg::InputGamepad>(
        "/control/input_gamepad", 10, std::bind(&Driver::input_callback, this, _1));
}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Subscriber class
    rclcpp::spin(std::make_shared<Driver>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}
