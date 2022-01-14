/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_simulator.h"

#include <string>
#include <vector>

#define _USE_MATH_DEFINES
#include <cmath>

// Constructor
ArmSimulator::ArmSimulator() : Node("arm_simulator")
{
    // Initialise constants
    // Will eventually be done in arm_core node and then inherited here
    num_joints = 6;
    timer_period = 200ms;
    
    // Set up the joint names
    joints.name = std::vector<std::string> {"base-rotation", "shoulder", "elbow", "wrist-1", "wrist-2", "wrist-3"};
    // Initial state of the joints
    joints.header.stamp = this->now();
    joints.position = std::vector<double> {0, 90, 0, 0, 0, 0};
    joints.velocity = std::vector<double> (num_joints);
    joints.effort = std::vector<double> (num_joints);
    
    // Create the subscription
    outputs_subscription = this->create_subscription<sensor_msgs::msg::JointState>(
        "/control/cmd_outputs", 10, std::bind(&ArmSimulator::subscriber_callback, this, _1)
    );

    // Create the publisher timer. Controls rate of publihsing to /resolvers topic
    publisher_timer = this->create_wall_timer(
        timer_period, std::bind(&ArmSimulator::publisher_callback, this)
    );

    // Create the publisher
    resolver_publisher = this->create_publisher<sensor_msgs::msg::JointState>(
        "/control/resolvers", 10
    );
}

// Convert a Real angle into the equivalent angle in [0, 2pi)
double ArmSimulator::wrap_to_2pi(double angle){
    angle = fmod(angle, 2*M_PI);
    if (angle < 0)
        angle += 2*M_PI;
    return angle;
}

// Use the current joint velocities to integrate the joint positions up to the current time
void ArmSimulator::update_joint_positions()
{
    // Get the current time
    rclcpp::Time current_time = this->now();

    // Update positions using duration from last recorded time to current time, and last velocity
    // Assumes joints have been moving at the last velocity they were told to
    rclcpp::Duration duration = current_time - joints.header.stamp;
    for(int i = 0; i < num_joints; i++){
        if (joints.velocity[i] != 0){
            joints.position[i] = wrap_to_2pi(joints.position[i] + joints.velocity[i] * duration.seconds());
        }   
    }
    // Update the header to record the time that was integrated to
    joints.header.stamp = current_time;
}


// Receive joint velocity command, update internal joint velocities
void ArmSimulator::subscriber_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
{    
    // Integrate the joint velocities up to the current time using the previous velocity
    update_joint_positions();
    // Update the stored velocity
    joints.velocity = msg->velocity;
}


// Create fake resolver output by integrating the joint positions up to the current time
void ArmSimulator::publisher_callback()
{
    // Integrate the joint positions up to the current time
    update_joint_positions();
    // Publish the current state of the joints
    resolver_publisher->publish(joints);
}


// Main function called by ros2 run
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Initialise and run the node
    rclcpp::spin(std::make_shared<ArmSimulator>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}