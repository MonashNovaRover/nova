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
  - /control/input_joystick_l          [core/InputJoystick]         [Subscribed]
  - /control/input_joystick_r          [core/InputJoystick]         [Subscribed]
  - /control/endeffector_input         [core/EndEffectorInput]      [Published]
  - /control/task_velocity             [sensor_msgs/TwistStamped]   [Published]
  - /control/input_joint_velocities    [sensor_msgs/JointState]     [Published]
  - /control/arm_control_scheme        [core/ArmControlScheme]      [Published]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Jess Hepworth, Jory Braun
CREATION:	02/12/2021
EDITED:		20/04/2022
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
#include "core/msg/endeffector_input.hpp"
#include "core/msg/arm_control_scheme.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"

// Use the standard namespaces
using namespace std::chrono_literals;
using std::placeholders::_1;


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
    rclcpp::TimerBase::SharedPtr control_scheme_timer;

    // Stores the publishers for arm inputs
    rclcpp::Publisher<core::msg::EndEffectorInput>::SharedPtr endeffector_publisher;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_vel_publisher;
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr task_vel_publisher;
    rclcpp::Publisher<core::msg::ArmControlScheme>::SharedPtr control_scheme_publisher;

    // Stores the subscribers to the joystick inputs
    rclcpp::Subscription<core::msg::InputJoystick>::SharedPtr joystick_l_subscription;
    rclcpp::Subscription<core::msg::InputJoystick>::SharedPtr joystick_r_subscription;

    // Stores messages to be published
    sensor_msgs::msg::JointState joint_velocities;
    geometry_msgs::msg::TwistStamped task_velocities;
    core::msg::ArmControlScheme control_scheme;

    // Store state of last-received messages
    core::msg::InputJoystick joystick_l;
    core::msg::InputJoystick joystick_r;

    // Store speed multipliers for different joystick inputs
    typedef struct {
        // Multiplier for all inputs
        // Tune this to adjust the max velocity of all joints
        float all_inputs = 0.70;
        // Separate multipliers for each set of inputs
        // Tune these so joints move at reasonable speeds relative to each other
        float wrist_joints = 1.20;
        float ik_linear = 0.50;
        float ik_angular = 0.85;
    } SpeedMultipliers;
    SpeedMultipliers speed_multipliers;

    //------------------------------------------------------------//
    private:

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_l_callback (const core::msg::InputJoystick::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_r_callback (const core::msg::InputJoystick::SharedPtr msg);
    
    /// @brief      Deadline callback for joystick subscriptions
    ///             Resets internal joystick state
    void joystick_deadline_callback();
    
    /// @brief      Publishes end effector input message
    void publish_endeffector_inputs ();

    /// @brief      Publishes desired joint velocities
    ///             Published as a joint-space vector in rad/s
    void publish_joint_vel ();

    /// @brief      Publishes desired task velocity
    ///             Published as a twist vector in m/s and rad/s
    void publish_task_vel ();

    /// @brief      Publishes control scheme data
    void publish_control_scheme ();

    /// @brief      Calculates a direction from a fraction
    /// @param      value - A fraction to be converted to a direction
    /// @returns    The calculated direction (-1, 0 or 1) 
    float calculate_direction (float value);

    /// @brief      Obtains postive scaling factor from slider input
    /// @param      value - number in range [-1, 1] to map to [0, 1]
    /// @returns    The new scale factor in range [0, 1]
    float scale_speed (float value);
       
    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    ArmInputs();
    
};
