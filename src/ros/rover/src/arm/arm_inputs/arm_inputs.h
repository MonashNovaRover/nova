#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class reads data from the raw joystick inputs
    and converts them to arm input messages.
This does not interface with the CMD library, but
    instead can be run on the base station to send
    arm data across the network.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: arm_inputs
TOPICS:
  - /control/input_joystick_l   [InputJoystick]     [Subscribed]
  - /control/input_joystick_r   [InputJoystick]     [Subscribed]
  - /control/arm_input          [ArmInput]          [Published]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Jess Hepworth
CREATION:	02/12/2021
EDITED:		18/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Add in additional inputs for linear actuate
 - Test with CMD code with subscriber
 - Naming of joint_velocities topic to not clash with joint_velocities_ik
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"
// Include messages types
#include "core/msg/input_joystick.hpp"
#include "core/msg/arm_input.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"

// Use the standard namespaces
using namespace std::chrono_literals;
using std::placeholders::_1;

// Get shared arm info
#include "arm_core.h"

/* 
Arm input class that handles input data from joysticks and publishes 
task and joint space velocities 
*/
class ArmInputs : public rclcpp::Node {

    //------------------------------------------------------------//
    private:

    // Stores the loop timers for the update functions
    rclcpp::TimerBase::SharedPtr timer;
    rclcpp::TimerBase::SharedPtr timer_joint;
    rclcpp::TimerBase::SharedPtr timer_task;

    // Stores a counter for each step
    size_t count;

    // Stores the publishers for arm inputs
    rclcpp::Publisher<core::msg::ArmInput>::SharedPtr arm_publisher;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_vel_publisher;
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr task_vel_publisher;


    // Stores the subscribers to the joystick inputs
    rclcpp::Subscription<core::msg::InputJoystick>::SharedPtr joystick_l_subscription;
    rclcpp::Subscription<core::msg::InputJoystick>::SharedPtr joystick_r_subscription;

    // Stores messages to be published
    sensor_msgs::msg::JointState joint_velocities;
    geometry_msgs::msg::TwistStamped task_velocities;

    // Stores task space inputs
    float task_velocity[NUM_JOINTS];

    // Stores joint space inputs
    float joint_velocity[NUM_JOINTS];

    // flag for IK on wrist 
    bool IK_wrist = false;

    // flag for IK on lower joints
    bool IK_lower_joints = false;

    // Stores end effector actuation data
    float end_effector_actuation;

    // Stores linear actuator data
    float linear_actuation;

    // Stores lunar construction data
    int lunar_construction_left;
    int lunar_construction_right;
 
    // Stores variable for inputs (i.e. speeds)
    float speed_multiplier;

    //------------------------------------------------------------//
    private:

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_l_callback (const core::msg::InputJoystick::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_r_callback (const core::msg::InputJoystick::SharedPtr msg);

    /// @brief      Function for publishing arm input message
    void publish_arm_inputs ();

    /// @brief      Function for publishing joint velocity
    void publish_joint_vel ();

    /// @brief      Function for publishing task velocity
    void publish_task_vel ();

    /// @brief      Function for calculating a direction from a fraction
    /// @param      value - A fraction to be converted to a direction
    /// @returns    The calculated direction (-1, 0 or 1) 
    float calculate_direction (float value);

    /// @brief      Function for obtaining postive scaling factor from slider input
    /// @param      value - number in range [-1, 1] to map to [0, 1]
    /// @returns    The new scale factor in range [0, 1]
    float scale_speed (float value);

    
    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    ArmInputs();
    
};