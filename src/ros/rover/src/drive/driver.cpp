/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios, Josh Cherubino
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include math library
#include <math.h>

// Include the header file
#include "driver.h"
#include "print/print.h"
#include <iostream>

// Sends commands to the wheels
void Driver::send_commands (const core::msg::DriveInput::SharedPtr msg) {
    
    // Check if wheels should spin
    if (msg->speed != 0.0 || msg->steer != 0.0) {

        // Reset the zero flag
        stopped_sent = false;

        // If no streer, just spin with speed
        if (msg->steer == 0) {
            for (Wheel* wheel : wheels)
                wheel->spin(msg->speed);

            return;
        }

        // Otherwise, calculate the speed

        // Store a new array
        float distances[NUM_WHEELS];
        float tangents[NUM_WHEELS];

        // Gets the locas distance
        float locas = get_locas_distance(msg->steer);
        float max_distance = 0;
        float max_tangent = 0;

        // Calculate the max distance to the wheels and store them
        for (int i = 0; i < NUM_WHEELS; i++) {
            float dist = get_wheel_distance(wheels[i]->get_id(), locas);
            distances[i] = dist;
            if (dist > max_distance) max_distance = dist;          

            float tangent = get_tangent_scale(wheels[i]->get_id(), locas);
            tangents[i] = tangent;
            if (tangent > max_tangent) max_tangent = tangent;
        }

        // Calculate the speed based on the distance
        //std::cout << distances[0] << ", " << distances[1] << ", " << distances[2] << ", " << distances[3] << ", " << distances[4] << ", " << distances[5] << std::endl;
        //std::fflush(stdout);

        for (int i = 0; i < NUM_WHEELS; i++) {
            // Calculate the velocity of wheel
            float vel = msg->speed * distances[i] / max_distance;
            vel *= tangents[i] / max_tangent;

            if (abs(locas) < CHASSIS_SEPARATION / 2.0) {
                if (locas < 0)
                    if (i > 2) vel *= -1.0;
                if (locas > 0)
                    if (i <= 2) vel *= -1.0;
            }

            // Send the velocities to the wheels
            wheels[i]->spin(vel);
        }

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

    // Enable handbraking based on the thumb buttons
    if (msg->connected && msg->btn_thumb_l_state == 1) {
        if (!handbrake) Print::print("Handbrake Enabled", C_MODE);
        handbrake = true;
    }
    
    // Disable Handbrake
    else if (msg->connected && msg->btn_thumb_r_state == 1) {
        if (handbrake) Print::print("Handbrake Disabled", C_MODE);
        handbrake = false;
    }

    // Enable  autonomous
    if (msg->connected && msg->btn_a_state == 1) {
        if (!is_autonomous) Print::print("Mode: Autonomous", C_MODE);
        is_autonomous = true;
    }

    // Disable autonomous
    else if (msg->connected && msg->btn_b_state == 1) {
        if (is_autonomous) Print::print("Mode: Manual", C_MODE);
        is_autonomous = false;
    }
}


// Main constructor that sets up the node
Driver::Driver() 
  : Node("drive_sub"), count(0) {

    // Output set-up messages
    Print::title("DRIVER");
    Print::print("", true);

    // Initialise the wheels in the correct direction
    for (int i = 0; i < NUM_WHEELS; i++) {
        bool left = i < NUM_WHEELS / 2;
        wheels[i] = new Wheel (i + 1, left);
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



float Driver::get_locas_distance (float steer) {
    if (steer == 0) return 0; // Maximum and will be ignored

    // Return the calculation
    return (1.0 / steer) - ((steer < 0.0) ? -1.0 : 1.0);
}


float Driver::get_wheel_distance (int id, float locas) {
    // Calculate the y component
    float y = 0;
    if (id == 1 || id == 4) y = WHEEL_SEPARATION;
    if (id == 3 || id == 6) y = -WHEEL_SEPARATION;

    float wheel_x = (CHASSIS_SEPARATION / 2.0) * ((id <= 3) ? -1.0 : 1.0);

    // Calculate the x component
    float x = locas - wheel_x;

    // Find pythagorus distance
    return sqrt(pow(x, 2) + pow(y, 2));
}


float Driver::get_tangent_scale (int id, float locas) {
    // Calculate the y component
    float y = 0;
    if (id == 1 || id == 4) y = WHEEL_SEPARATION;
    if (id == 3 || id == 6) y = -WHEEL_SEPARATION;

    float wheel_x = (CHASSIS_SEPARATION / 2.0) * ((id <= 3) ? -1.0 : 1.0);

    // Calculate the x component
    float x = locas - wheel_x;

    // Find pythagorus distance
    return sqrt(1.0 + pow(y / x, 2));
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
