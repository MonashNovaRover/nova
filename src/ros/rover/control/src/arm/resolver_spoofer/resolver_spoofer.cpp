/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "resolver_spoofer.h"

#include "arm_messages.h"
#include "print/print.h"
#include "config/rosconfig.h"

#define _USE_MATH_DEFINES
#include <cmath>

// Use the standard namespace for subscribers
using std::placeholders::_1;


void ResolverSpoofer::start_node()
{
    // Set up the joints structure
    joints = ArmMessages::get_empty_joint_state(arm_config_info.joint_names);
    
    // Initial integration time
    last_integration_time = this->now();

    // Set the discontinuity angles for each joint
    joint_discontinuity_angles = std::vector<float> (arm_config_info.num_joints);
    for (uint16_t i = 0; i < arm_config_info.num_joints; i++){
        float limit_lower = arm_config_info.joint_limits_lower[i];
        float limit_upper = arm_config_info.joint_limits_upper[i];
        joint_discontinuity_angles[i] = wrap_to_2pi((limit_lower + limit_upper) / 2 + M_PI);
    }
    
    // Create the subscription
    outputs_subscription = this->create_subscription<sensor_msgs::msg::JointState>(
        "/control/joint_velocities",
        rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline),
        std::bind(&ResolverSpoofer::subscriber_callback, this, _1)
    );

    // Create the publisher timer. Controls rate of publihsing to /resolvers topic
    publisher_timer = this->create_wall_timer(
        ROSTimers::arm_resolvers, std::bind(&ResolverSpoofer::publisher_callback, this)
    );

    // Create the publisher
    resolver_publisher = this->create_publisher<sensor_msgs::msg::JointState>(
        "/electronics/resolvers", 10
    );

    // Output set-up messages
    Print::title("RESOLVER SPOOFER");
    Print::print("Subscribed Topics:");
    Print::print("/control/joint_velocities   [sensor_msgs/JointState]", 1);
    Print::print("Published Topics:");
    Print::print("/electronics/resolvers      [sensor_msgs/JointState]", 1);
    Print::print("", true);
}

// Convert a Real angle into the equivalent angle in [0, 2pi)
inline double ResolverSpoofer::wrap_to_2pi(double angle){
    angle = fmod(angle, 2*M_PI);
    if (angle < 0)
        angle += 2*M_PI;
    return angle;
}

// Move the discontinuity angle from 2pi to some specified angle
inline double ResolverSpoofer::move_discontinuity(double angle, double discontinuity_angle){
    return angle - 2*M_PI * (angle >= discontinuity_angle);
}

// Use the current joint velocities to integrate the joint positions up to the current time
void ResolverSpoofer::update_joint_positions()
{
    // Get the current time
    rclcpp::Time current_time = this->now();

    // Update positions using duration from last recorded time to current time, and last velocity
    // Assumes joints have been moving at the last velocity they were told to
    rclcpp::Duration duration = current_time - last_integration_time;
    for (uint16_t i = 0; i < arm_config_info.num_joints; i++){
        if (joints.velocity[i] != 0){
            double angle = wrap_to_2pi(joints.position[i] + joints.velocity[i] * duration.seconds());
            joints.position[i] = move_discontinuity(angle, joint_discontinuity_angles[i]);
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