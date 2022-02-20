/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "resolver_spoofer.h"

#include "arm_core.h"
#include "print/print.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "../hacky_defines.h"

// Constructor
ResolverSpoofer::ResolverSpoofer() : Node("resolver_spoofer")
{
    // Initialise constants
    timer_period = 200ms;
    
    // Set up the joints structure
    joints = ArmCore::get_empty_joint_state(hack::JOINT_NAMES);
    
    // Initial integration time
    last_integration_time = this->now();
    
    // Create the subscription
    outputs_subscription = this->create_subscription<sensor_msgs::msg::JointState>(
        "/control/cmd_outputs", 10, std::bind(&ResolverSpoofer::subscriber_callback, this, _1)
    );

    // Create the publisher timer. Controls rate of publihsing to /resolvers topic
    publisher_timer = this->create_wall_timer(
        timer_period, std::bind(&ResolverSpoofer::publisher_callback, this)
    );

    // Create the publisher
    resolver_publisher = this->create_publisher<sensor_msgs::msg::JointState>(
        "/electronics/resolvers", 10
    );

    // Output set-up messages
    Print::title("RESOLVER SPOOFER");
    Print::print("Subscribed Topics:");
    Print::print("/control/cmd_outputs        [sensor_msgs/JointState]", 1);
    Print::print("Published Topics:");
    Print::print("/electronics/resolvers      [sensor_msgs/JointState]", 1);
    Print::print("", true);
}

// Convert a Real angle into the equivalent angle in [0, 2pi)
double ResolverSpoofer::wrap_to_2pi(double angle){
    angle = fmod(angle, 2*M_PI);
    if (angle < 0)
        angle += 2*M_PI;
    return angle;
}

// Use the current joint velocities to integrate the joint positions up to the current time
void ResolverSpoofer::update_joint_positions()
{
    // Get the current time
    rclcpp::Time current_time = this->now();

    // Update positions using duration from last recorded time to current time, and last velocity
    // Assumes joints have been moving at the last velocity they were told to
    rclcpp::Duration duration = current_time - last_integration_time;
    for(unsigned int i = 0; i < joints.name.size(); i++){
        if (joints.velocity[i] != 0){
            joints.position[i] = wrap_to_2pi(joints.position[i] + joints.velocity[i] * duration.seconds());
        }   
    }
    // Record the time that was integrated to
    last_integration_time = current_time;
}

// Receive joint velocity command, update internal joint velocities
void ResolverSpoofer::subscriber_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
{    
    // Integrate the joint velocities up to the current time using the previous velocity
    update_joint_positions();
    // Update the stored velocity
    joints.velocity = msg->velocity;
}

// Create fake resolver output by integrating the joint positions up to the current time
void ResolverSpoofer::publisher_callback()
{
    // Integrate the joint positions up to the current time
    update_joint_positions();
    // Update the header
    joints.header.stamp = this->now();
    // Publish the current state of the joints
    resolver_publisher->publish(joints);
}


// Main function called by ros2 run
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Initialise and run the node
    rclcpp::spin(std::make_shared<ResolverSpoofer>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}