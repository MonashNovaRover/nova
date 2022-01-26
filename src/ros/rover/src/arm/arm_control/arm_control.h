#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This class is determines what velocity values will be given 
    to the CMDs, based on joint limits

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: arm_control
TOPICS:
  - /control/joint_velocities_ik    [sensor_msgs/JointState]     [Subscribed]
  - /control/joint_velocities       [sensor_msgs/JointState]     [Subscribed]
  - /control/resolvers              [sensor_msgs/JointState]     [Subscribed]
  - /control/cmd_outputs            [sensor_msgs/JointState]     [Published]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Jess Hepworth
CREATION:	26/01/2022
EDITED:		26/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"
// Include messages types
#include "core/msg/arm_input.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

// Use the standard namespaces
using namespace std::chrono_literals;
using std::placeholders::_1;

// Get shared arm info
#include "arm_core.h"

/* 
Arm control class that handles velocities to be given to CMDs
*/
class ArmControl : public rclcpp::Node {

    //------------------------------------------------------------//
    private:

    // Stores the loop timers for the update functions
    rclcpp::TimerBase::SharedPtr timer;
    rclcpp::TimerBase::SharedPtr timer_joint;
    rclcpp::TimerBase::SharedPtr timer_task;

    // Stores a counter for each step
    size_t count;

    // Stores the publisher for desired CMD outputs
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr CMD_outputs_publisher;

    // Stores the subscribers to the joints velocities 
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_vel_ik_subscription;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_vel_subscription;
    //Store the subscribers to the resolvers
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr resolvers_subscription;

    // Stores task space inputs
    float joint_velocity_ik[NUM_JOINTS];

    // Stores joint space inputs
    float joint_velocity[NUM_JOINTS];

    //------------------------------------------------------------//
    private:

    /// @brief      Callback function when IK joint velocities are received.
    /// @param      msg - A pointer to the input message
    void joint_vel_ik_callback (const sensor_msgs::msg::JointState::SharedPtr msg);

    /// @brief      Callback function when non-IK joint velocities are received.
    /// @param      msg - A pointer to the input message
    void joint_vel_callback (const sensor_msgs::msg::JointState::SharedPtr msg);

    /// @brief      Callback function when resolver values are received.
    /// @param      msg - A pointer to the input message
    void resolver_callback (const sensor_msgs::msg::JointState::SharedPtr msg);

    /// @brief      Function for publishing desired CMD outputs
    void publish_CMD_outputs ();

    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    ArmControl();
    
};