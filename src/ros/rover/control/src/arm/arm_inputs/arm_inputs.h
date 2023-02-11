#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class reads data from the raw joystick inputs
    and converts them to arm input messages.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: arm_inputs
TOPICS:
  - /control/input_joystick_l            [core/InputJoystick]          [Subscribed]
  - /control/input_joystick_r            [core/InputJoystick]          [Subscribed]
  - /control/endeffector_input           [core/EndEffectorInput]       [Published]
  - /control/joystick_joint_velocities   [sensor_msgs/JointState]      [Published]
  - /control/joystick_twist              [geometry_msgs/TwistStamped]  [Published]
  - /control/arm_control_scheme          [core/ArmControlScheme]       [Published]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Jess Hepworth, Jory Braun
CREATION:	02/12/2021
EDITED:		07/10/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"

// Include messages types
#include "core/msg/input_joystick.hpp"
#include "core/msg/end_effector_input.hpp"
#include "core/msg/arm_control_scheme.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"

// Include libraries
#include "arm_config_info_client.h"

// Position control enable override
#define POSITION_CONTROL_ENABLE 0

/* 
Arm input class that handles input data from joysticks and publishes 
task and joint space velocities 
*/
class ArmInputs : public ArmConfigInfoClient
{
    //------------------------------------------------------------//
    private:

    // Stores the publishers for arm inputs
    rclcpp::Publisher<core::msg::EndEffectorInput>::SharedPtr endeffector_pub;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_velocities_pub;
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr twist_pub;
    rclcpp::Publisher<core::msg::ArmControlScheme>::SharedPtr control_scheme_pub;

    // Stores messages to be published
    sensor_msgs::msg::JointState joint_velocities;
    geometry_msgs::msg::TwistStamped twist;
    core::msg::ArmControlScheme control_scheme;

    // Store state of last-received messages
    core::msg::InputJoystick joystick_l;
    core::msg::InputJoystick joystick_r;

    // Store speed multipliers for different joystick inputs
    typedef struct {
        // Multiplier for all inputs
        // Tune this to adjust the max velocity of all joints
        float all_inputs = 0.30;
        // Separate multipliers for each set of inputs
        // Tune these so joints move at reasonable speeds relative to each other
        float wrist_joints = 1.20;
        float ik_linear = 0.50;
        float ik_angular = 0.85;
    } SpeedMultipliers;
    SpeedMultipliers speed_multipliers;

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
    void publish_joint_velocities ();

    /// @brief      Publishes desired task velocity
    ///             Published as a twist vector in m/s and rad/s
    void publish_twist ();

    /// @brief      Publishes desired joint velocities and task velocity
    void publish_inputs();

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

    /// @brief      Application setup function. Starts publishers, subscribers and initialises members
    void start_node() override;

    //------------------------------------------------------------//
    public:

    /// @brief      Constructor. Starts the node
    ArmInputs() : ArmConfigInfoClient("arm_inputs"){}
    
};
