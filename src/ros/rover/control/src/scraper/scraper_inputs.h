#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class reads data from the raw joystick inputs
    and converts them to scraper input messages.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: arm_inputs
TOPICS:
  - /control/input_joystick_l            [core/InputJoystick]          [Subscribed]
  - /control/input_joystick_r            [core/InputJoystick]          [Subscribed]
  - /control/scraper_arm_velocity        [sensor_msgs/JointState]      [Published]
  - /control/scraper_scoop_velocity      [geometry_msgs/TwistStamped]  [Published]
  - /control/arm_control_scheme          [core/ArmControlScheme]       [Published]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Manika Goyal
CREATION:	03/03/2023
EDITED:		03/03/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"

// Include messages types
#include "core/msg/input_joystick.hpp"
#include "core/msg/arm_control_scheme.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

// Include libraries
#include "arm_config_info_client.h"

/* 
Scraper input class that handles input data from joysticks and publishes 
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
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_velocities_pub;
    rclcpp::Publisher<core::msg::ScraperControlScheme>::SharedPtr control_scheme_pub;

    // Stores the subscribers to the joystick inputs
    rclcpp::Subscription<core::msg::InputJoystick>::SharedPtr joystick_l_sub;
    rclcpp::Subscription<core::msg::InputJoystick>::SharedPtr joystick_r_sub;

    // Stores messages to be published
    sensor_msgs::msg::JointState joint_velocities;
    core::msg::ScraperControlScheme scraper_control_scheme;

    // Store state of last-received messages
    core::msg::InputJoystick joystick_l;
    core::msg::InputJoystick joystick_r;

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_l_callback (const core::msg::InputJoystick::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void joystick_r_callback (const core::msg::InputJoystick::SharedPtr msg);
    
    /// @brief      Deadline callback for joystick subscriptions
    ///             Resets internal joystick state
    void joystick_deadline_callback();

    /// @brief      Publishes desired joint velocities
    ///             Published as a joint-space vector in rad/s
    void publish_joint_velocities ();

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
    ScraperInputs() : ScraperConfigInfoClient("scraper_inputs"){}
    
};
