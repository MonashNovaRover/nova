#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class reads data from the raw joystick inputs
    and converts them to arm input messages.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: arm_inputs
TOPICS:
  - /inputs/input_joystick_l            [input_interfaces/InputJoystick]          [Subscribed]
  - /inputs/input_joystick_r            [input_interfaces/InputJoystick]          [Subscribed]
  - /inputs/input_keyboard              [core/InputKeyboard]          [Subscribed]
  - /arm/endeffector_input           [arm_interfaces/EndEffectorInput]       [Published]
  - /arm/joystick_joint_velocities   [sensor_msgs/JointState]      [Published]
  - /arm/joystick_twist              [geometry_msgs/TwistStamped]  [Published]
  - /arm/arm_control_scheme          [arm_interfaces/ArmControlScheme]       [Published]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Jess Hepworth, Jory Braun, Matthew Gu
CREATION:	02/12/2021
EDITED:		07/12/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Note:
Current implementation updates all devices regardless 
of which ones are connected
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"

// Include messages types
#include "input_interfaces/msg/input_joystick.hpp"
#include "input_interfaces/msg/input_keyboard.hpp"
#include "arm_interfaces/msg/end_effector_input.hpp"
#include "arm_interfaces/msg/arm_control_scheme.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"

// Include libraries
#include "arm_core/arm_config_info_client.h"

#include "arm_inputs/joystick_translate.h"
#include "arm_inputs/keyboard_translate.h"

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

    // Stores the loop timers for the update functions
    rclcpp::TimerBase::SharedPtr endeffector_pub_timer;
    rclcpp::TimerBase::SharedPtr inputs_pub_timer;
    rclcpp::TimerBase::SharedPtr control_scheme_pub_timer;

    // Stores the publishers for arm inputs
    rclcpp::Publisher<arm_interfaces::msg::EndEffectorInput>::SharedPtr endeffector_pub;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_velocities_pub;
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr twist_pub;
    rclcpp::Publisher<arm_interfaces::msg::ArmControlScheme>::SharedPtr control_scheme_pub;

    // Stores the subscribers to the joystick inputs
    rclcpp::Subscription<input_interfaces::msg::InputJoystick>::SharedPtr joystick_l_sub;
    rclcpp::Subscription<input_interfaces::msg::InputJoystick>::SharedPtr joystick_r_sub;
    rclcpp::Subscription<input_interfaces::msg::InputKeyboard>::SharedPtr keyboard_sub;

    // Stores messages to be published
    arm_interfaces::msg::EndEffectorInput end_effector_inputs;
    sensor_msgs::msg::JointState joint_velocities;
    geometry_msgs::msg::TwistStamped twist;
    arm_interfaces::msg::ArmControlScheme control_scheme;

    // set to true when the joystick input is prioritised
    bool joystick_override;

    // Store the input device translators
    JoystickTranslate joystick_translate;
    KeyboardTranslate keyboard_translate;

    /// @brief     Input device selected
    InputDevice* input_device;

    /// @brief     Selects the input device to use
    /// @return    A pointer to the input device
    InputDevice* select_input_device();

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_l_callback (const input_interfaces::msg::InputJoystick::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_r_callback (const input_interfaces::msg::InputJoystick::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void keyboard_callback (const input_interfaces::msg::InputKeyboard::SharedPtr msg);
    
    /// @brief      Deadline callback for joystick subscriptions
    ///             Resets internal input states
    void joystick_deadline_callback();

    /// @brief      Deadline callback for keyboard subscriptions
    ///             Resets internal input states
    void keyboard_deadline_callback();
    
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

    /// @brief      Application setup function. Starts publishers, subscribers and initialises members
    void start_node() override;

    //------------------------------------------------------------//
    public:

    /// @brief      Constructor. Starts the node
    ArmInputs();
    
};
