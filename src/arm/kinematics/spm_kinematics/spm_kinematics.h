#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class performs the spm Roll-Pithc-Yaw (RPY) integration, the
    spm IK, and the wrist joint angle differentiation.
It reads the resolver data, and desired Roll-Pitch-Yaw velocities, and
    outputs the desired wrist joint velocities.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: spm_kinematics
TOPICS:
  - /control/joint_velocities          [sensor_msgs/JointState]          [Subscribed]
  - /electronics/resolvers             [sensor_msgs/JointState]          [Subscribed]
  - /control/cmd_outputs               [sensor_msgs/JointState]          [Published]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S):   Alexander Li
CREATION:	 23/04/2022
EDITED:		 23/04/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - implement spm_fk
 - implement spm_ik
 - create publisher to cmd_outputs(?) (to where do we send the desired wrist joint velocities?)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS client library
#include "rclcpp/rclcpp.hpp"
// Include message types
#include "sensor_msgs/msg/joint_state.hpp"
// Include vector 
#include <vector>

/* 
Class which solves the spm kinematics
*/
class SpmKinematics : public rclcpp::Node
{
    //------------------------------------------------------------//
    private:

    // Period that indicates time since last publish
    std::chrono::milliseconds joint_velocities_timer_period;
    float time_since_last_publish;

    // Wrist geometry
    float beta;
    float gamma;
    float alpha[3];
    float eng[3];

    // Track internal state
    // Resolvers; current wrist joint positions
    float current_wrist_joint_pos[3];
    // SPM FK; current RPY positions (theta_X, theta_y, theta_z)
    float current_rpy_pos[3];
    float previous_rpy_pos[3];
    // desired RPY velocities from control/joint_velocities
    float desired_rpy_vel[3];
    // integrator; desired RPY positions
    float desired_rpy_pos[3];
    // SPM IK; desired wrist joint positions
    float desired_wrist_joint_pos[3];
    

    // Subscription to resolvers
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr resolver_sub;
    // Subscription to desired RPY velocities
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_velocities_sub;
    // Publisher to /control/cmd_outputs
    rclcpp::TimerBase::SharedPtr cmd_outputs_timer;
    rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr cmd_outputs_pub;


    /// @brief  Callback for resolver subscription
    ///         Updates the internal joint state, which is used for spm FK, and integrator and differentiator
    void resolver_callback(const sensor_msgs::msg::JointState::SharedPtr msg);

    /// @brief  Callback for joint_velocities subscription
    ///         
    void joint_velocities_callback(const sensor_msgs::msg::JointState::SharedPtr msg);

    /// @brief  Callback for cmd_outputs publihser timer
    ///         
    void publish_cmd_outputs();
    
    //------------------------------------------------------------//
    public:

    // desired wrist joint velocitites - output of the spm kinematics solver
    float desired_wrist_joint_vel[3];

    /// Constructor. Initialisers the solvers and starts the node
    SpmKinematics();

    /// @brief  Ultimate solver function for spm kinematics
    ///         Updates desired_wrist_joint_velocities -> this will be sent as a JointState message to cmd_outputs(?)
    ///         Or can be accessed in arm_kinematics.cpp
    ///         Calls numerous functions below in the process
    void solve();

    /// @brief  Function for spm fk
    ///         Takes in float array of wrist joint positions
    ///         Outputs float array of RPY
    ///         Calls v_to_rpy() function in the process
    float *spm_fk(const float wrist_joint_pos[3]);

    /// @brief  Function for spm ik
    ///         Takes in float array of desired RPY
    ///         Outputs float array of desired wrist joint positions
    ///         Calls rpy_to_v() function in the process
    float *spm_ik(const float desired_rpy_pos[3]);

    /// @brief  Function to integrate RPY
    ///         Takes in float array of current RPY, float array of desired RPY_VEL, and timestep
    ///         Outputs float array of desired RPY
    float *rpy_integrator(const float current_rpy[3], const float desired_rpy_vel[3], int time_step);

    /// @brief  Function to differentiate wrist joint position
    ///         Takes in float array of desired wrist joint positions, current wrist joint positions, and timestep
    ///         Outputs float array of desired wrist joint velocities
    float *joint_differentiator(const float desired_wrist_joint_pos[3], const float current_wrist_joint_pos[3], int time_step);
    
    /// @brief  Function for spm fk
    ///         Takes in a float array representing v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z, also takes in previous rpy
    ///         Outputs float array of RPY
    ///         Called by spm_fk()
    float *v_to_rpy(float v[9], float prev_rpy[3]);

    /// @brief  Function for spm ik
    ///         Takes in a float array of RPY
    ///         Outputs float array representing v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z
    ///         Called by spm_ik()
    float *rpy_to_v(float rpy[3]);

    /// @brief  Mathematics function for vector addition of length 3
    float *vector_addition(float x[3], float y[3])

    /// @brief  Mathematics function for vector addition of length 3, for 3 vectors
    float *vector_addition(float x[3], float y[3], float z[3])

    /// @brief  Mathematics function for vector scalar product of length 3
    float *vector_scalar_product(float v[3], float s);

    /// @brief  Mathematics function for dot product of vectors of length 3
    float vector_dot_product(float x[3], float y[3]);

    /// @brief  Mathematics function for cross product of vectors of length 3
    float *vector_cross_product(float x[3], float y[3]);

    /// @brief  Mathematics function for finding a custom metric between euler configurations
    float euler_metric(float theta_a[3], float theta_b[3]);
};