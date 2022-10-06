#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class transforms the task-space twist from the 
  joysticks to the rover frame
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: arm_twistmapper
TOPICS:
  - /control/arm_control_scheme        [core/ArmControlScheme]           [Subscribed]
  - /electronics/resolvers             [sensor_msgs/JointState]          [Subscribed]
  - /control/joystick_joint_velocities [sensor_msgs/JointState]          [Subscribed]
  - /control/joystick_twist            [geometry_msgs/TwistStamped]      [Subscribed]
  - /control/control_joints            [sensor_msgs/JointState]          [Published]
  - /control/control_twist             [geometry_msgs/TwistStamped]      [Published]
  - /control/control_pose              [geometry_msgs/TransformStamped]  [Published]
SERVICES:
  - /control/arm_reset_control_pose    [std_srvs/Trigger]                [Server]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	   control
AUTHOR(S):   Jory Braun
CREATION:	   25/09/2022
EDITED:		   07/10/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS client library
#include "rclcpp/rclcpp.hpp"

// Include message types
#include "core/msg/arm_control_scheme.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"
// Include service types
#include "std_srvs/srv/trigger.hpp"

// Include libraries
#include "arm_model.h"
#include "arm_kinematics.h"

// Use the standard namespaces
using namespace std::chrono_literals;
using std::placeholders::_1;
using std::placeholders::_2;

/*
Class which transforms the input twist to the rover coordinate frame
*/
class ArmTwistMapper : public rclcpp::Node
{
    //------------------------------------------------------------//
    private:

    // Subscribers
    rclcpp::Subscription<core::msg::ArmControlScheme>::SharedPtr control_scheme_sub;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr resolver_sub;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joystick_joint_velocities_sub;
    rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr joystick_twist_sub;
    
    // Loop timers for publishing
    std::chrono::milliseconds control_pub_timer_period;
    rclcpp::TimerBase::SharedPtr control_pub_timer;

    // Publishers
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr control_joints_pub;
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr control_twist_pub;
    rclcpp::Publisher<geometry_msgs::msg::TransformStamped>::SharedPtr control_pose_pub;

    // Services (servers)
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr arm_reset_control_pose_service;

    // State of last-received messages
    core::msg::ArmControlScheme control_scheme;
    sensor_msgs::msg::JointState joints;
    // Store joystick_joint_velocities in velocity section in control_joints_msg
    geometry_msgs::msg::TwistStamped joystick_twist;
    
    // Messages to be published (so only need to initialise once)
    sensor_msgs::msg::JointState control_joints_msg;
    geometry_msgs::msg::TwistStamped control_twist_msg;
    geometry_msgs::msg::TransformStamped control_pose_msg;

    // Arm model and solvers
    ArmModel* arm_model;
    ArmKinematics* arm_kinematics_solver;

    // Constant transforms
    // Transform joystick input directions to intuitive end-effector coordinates
    const KDL::Rotation ENDPOINT_INPUT_TRANSFORM_LINEAR;
    const KDL::Rotation ENDPOINT_INPUT_TRANSFORM_ANGULAR;

    // Control variables
    KDL::JntArray control_configuration;
    KDL::JntArray prev_control_velocities;
    KDL::Frame control_pose;
    KDL::Twist prev_control_twist;
    rclcpp::Time prev_time;

    /// @brief  Callback for control scheme subscription
    ///         Updates the internal control scheme, which is used to determine how to solve IK
    void control_scheme_callback(const core::msg::ArmControlScheme::SharedPtr msg);

    /// @brief  Callback for resolver subscription
    ///         Updates the internal joint state, which is later used to update the model
    void resolver_callback(const sensor_msgs::msg::JointState::SharedPtr msg);

    /// @brief  Callback for joystick joint velocities subscription
    ///         Updates the internal joystick joint velocities, which is used to calculate the control configuration
    void joystick_joint_velocities_callback(const sensor_msgs::msg::JointState::SharedPtr msg);
    /// @brief  Deadline callback for input joystick joint velocities subscription
    ///         Resets the internal joystick joint velocities
    void joystick_joint_velocities_deadline_callback();

    /// @brief  Callback for joystick twist subscription
    ///         Updates the internal joystick twist, which is used to calculate the control twist and pose
    void joystick_twist_callback(const geometry_msgs::msg::TwistStamped::SharedPtr msg);
    /// @brief  Deadline callback for input joystick twist subscription
    ///         Resets the internal joystick twist
    void joystick_twist_deadline_callback();

    /// @brief  Get the rover-frame twist given the current input twist and the selected control scheme
    KDL::Twist get_control_twist(const KDL::Twist& joystick_twist);

    /// @brief  Integrate the joint-space control configuration over the given timestep
    void update_control_configuration(const KDL::JntArray& control_velocities, double timestep);
    
    /// @brief  Integrate the task-space control twist over the given timestep
    void update_control_pose(const KDL::Twist& control_twist, double timestep);

    /// @brief  Callback for the control publisher timer
    ///         Publishes the task-space rover-frame control twist and control pose
    ///         Publishes the joint-space control configuration
    void publish_control_inputs();

    /// @brief  Set the control pose to the current position
    void reset_control_position();

    /// @brief  Set the control pose to the current orientation
    void reset_control_orientation();

    /// @brief  Callback for arm_reset_control_pose service
    ///         Reinitialises the position control
    void arm_reset_control_pose_callback(
        const std_srvs::srv::Trigger::Request::SharedPtr request,
        std_srvs::srv::Trigger::Response::SharedPtr response
    );
    
    //------------------------------------------------------------//
    public:

    /// Constructor. Starts publishers, subscribers and initialises members
    ArmTwistMapper();
    
};
