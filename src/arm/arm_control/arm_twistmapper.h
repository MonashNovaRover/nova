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
  - /control/input_task_velocity       [geometry_msgs/TwistStamped]      [Subscribed]
  - /control/task_velocity             [geometry_msgs/TwistStamped]      [Published]
  - /control/task_position             [geometry_msgs/TransformStamped]  [Published]
SERVICES:
  - /control/arm_reset_control_pose    [std_srvs/Trigger]                [Server]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	   control
AUTHOR(S):   Jory Braun
CREATION:	   25/09/2022
EDITED:		   02/10/2022
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
    rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr input_task_velocity_sub;
    
    // Loop timers for publishing
    std::chrono::milliseconds task_inputs_timer_period;
    rclcpp::TimerBase::SharedPtr task_inputs_timer;

    // Publishers
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr task_velocity_pub;
    rclcpp::Publisher<geometry_msgs::msg::TransformStamped>::SharedPtr task_position_pub;

    // Services (servers)
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr arm_reset_control_pose_service;

    // State of last-received messages
    core::msg::ArmControlScheme control_scheme;
    sensor_msgs::msg::JointState joints;
    geometry_msgs::msg::TwistStamped input_task_velocity;
    
    // Messages to be published (so only need to initialise once)
    geometry_msgs::msg::TwistStamped task_velocity;
    geometry_msgs::msg::TransformStamped task_position;

    // Arm model and solvers
    ArmModel* arm_model;
    ArmKinematics* arm_kinematics_solver;

    // Constant transforms
    // Transform joystick input directions to intuitive end-effector coordinates
    const KDL::Rotation endpoint_input_transform_linear;
    const KDL::Rotation endpoint_input_transform_angular;

    // Control variables
    KDL::Frame control_pose;
    KDL::Twist prev_control_twist;
    rclcpp::Time prev_time;

    /// @brief  Callback for control scheme subscription
    ///         Updates the internal control scheme, which is used to determine how to solve IK
    void control_scheme_callback(const core::msg::ArmControlScheme::SharedPtr msg);

    /// @brief  Callback for resolver subscription
    ///         Updates the internal joint state, which is later used to update the model
    void resolver_callback(const sensor_msgs::msg::JointState::SharedPtr msg);

    /// @brief  Callback for task velocity subscription
    ///         Updates the internal task velocity, which is later used to calculate the inverse kinematics
    void input_task_velocity_callback(const geometry_msgs::msg::TwistStamped::SharedPtr msg);
    /// @brief  Deadline callback for input task velocity subscription
    ///         Resets the internal task velocity
    void input_task_velocity_deadline_callback();

    /// @brief  Get the rover-frame twist given the current input twist and the selected control scheme
    KDL::Twist get_control_twist(const KDL::Rotation& endpoint_coord_transform);

    /// @brief  Integrate the control twist over the given timestep to get a new control pose
    void update_control_pose(const KDL::Twist& control_twist, const KDL::Frame& endpoint_frame, double timestep);

    /// @brief  Callback for task_velocity publisher timer
    ///         Calculates the rover-frame control twist from the task-space input, publishes to task_velocity
    ///         Calculates the rover-frame control pose, publishes to task_position
    void publish_task_inputs();

    /// @brief  Set the control pose to the current position and reinitialise the position control
    void reset_control_pose();

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
