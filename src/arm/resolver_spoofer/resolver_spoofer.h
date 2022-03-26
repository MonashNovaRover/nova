#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class simulates resolver feedback from operating the CMDs.
It can be used in place of the arm_driver for when no physical
  arm is connected.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: arm_model
TOPICS:
  - /control/joint_velocities  [sensor_msgs/JointState]    [Subscribed]
  - /electronics/resolvers     [sensor_msgs/JointState]    [Published]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S): Jory Braun
CREATION:	 18/12/2021
EDITED:		 22/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Test with python plotter
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS client library
#include "rclcpp/rclcpp.hpp"
// Include message types
#include "sensor_msgs/msg/joint_state.hpp"

// Use the standard namespaces
// For publishers
using namespace std::chrono_literals;
// For subscribers
using std::placeholders::_1;


class ResolverSpoofer : public rclcpp::Node
{
    //------------------------------------------------------------//
    private:

    // Period at which to publish to /control/resolvers
    std::chrono::milliseconds timer_period;

    // Track internal state of all joints
    // Includes joint names, position, velocity, effort and the corresponding timestamp
    // Set initial value using arm_core
    sensor_msgs::msg::JointState joints;
    // Track the time that the joint velocities were last integrated to.
    // This is distinct from the timestamp in joints, which represents the time each message was sent
    rclcpp::Time last_integration_time;

    // Store the angles for each joint at which to set the periodic angle discontinuity
    std::vector<float> joint_discontinuity_angles;

    // Subscriber to listen for output joint velocity commands
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr outputs_subscription;
    // Timer for publishing to /electronics/resolvers
    rclcpp::TimerBase::SharedPtr publisher_timer;
    // Publisher to resolvers
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr resolver_publisher;

    /// Converts a Real angle into the equivalent angle in [0, 2pi)
    double wrap_to_2pi(double angle);

    /// Move the perioidic angular discontinuity from 2pi to some specified angle
    double move_discontinuity(double angle, double discontinuity_angle);
    
    /// Integrates the joint velocities up to the current time
    void update_joint_positions();

    /// @brief  Callback for subscriber
    ///         Updates internal joint velocities
    void subscriber_callback(const sensor_msgs::msg::JointState::SharedPtr msg);
    
    /// @brief  Callback for publisher timer
    ///         Updates position base don current velocity, publishes to resolvers
    void publisher_callback();
    
    //------------------------------------------------------------//
    public:

    /// Default constructor
    ResolverSpoofer();

};