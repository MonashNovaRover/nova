#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class performs the spm Roll-Pithc-Yaw (RPY)(strictly speaking, theta_x, theta_y, theta_Z) integration, the
    spm IK, and the wrist joint angle differentiation.
It reads the resolver data, and desired Roll-Pitch-Yaw velocities, and
    outputs the desired wrist joint velocities.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S):   Alexander Li
CREATION:	 23/04/2022
EDITED:		 23/04/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - implement spm_fk
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include vector 
#include <vector>
#include <cmath>
#define _USE_MATH_DEFINES

/* 
Class which solves the spm kinematics
*/
class SpmKinematics
{
    //------------------------------------------------------------//
    private:

    // Wrist geometry
    const double beta = 54.74 * M_PI / 180;
    const double gamma = 54.74 * M_PI / 180;
    const double alpha[3] = {M_PI/2, M_PI/2, 2*asin(sin(beta)*cos(M_PI/6))};
    const double eta[3] = {0.0, 2*M_PI/3, 4*M_PI/3};

    // Track internal state
    const int num_base_joints = 3;
    std::vector<double> current_rpy_pos = {0, 0, 0};
    std::vector<double> previous_rpy_pos;
    // Resolvers; current wrist joint positions
    double current_wrist_joint_pos[3];
    // desired RPY velocities from control/joint_velocities
    double desired_rpy_vel[3];
    
    //------------------------------------------------------------//
    public:

    /// Constructor. Initialisers the solvers and starts the node
    SpmKinematics();

    /// @brief  Ultimate solver function for spm kinematics
    ///         Takes in current wrist joint positions (resolvers), and desired rpy velocities, and timestep
    ///         Outputs desired wrist joint velocities
    ///         Or can be accessed in arm_kinematics.cpp
    ///         Calls numerous functions below in the process
    std::vector<double> solve(std::vector<double> current_wrist_joint_pos, std::vector<double> desired_rpy_vel, int time_step);

    /// @brief  Function for spm fk
    ///         Takes in vector of wrist joint positions
    ///         Outputs vector of RPY
    ///         Calls v_to_rpy() function in the process
    std::vector<double> spm_fk(std::vector<double> current_wrist_joint_pos);

    /// @brief  Function for spm ik
    ///         Takes in vector of desired RPY
    ///         Outputs vector of desired wrist joint positions
    ///         Calls rpy_to_v() function in the process
    std::vector<double> spm_ik(std::vector<double> desired_rpy_pos);

    /// @brief  Function to integrate RPY
    ///         Takes in vector of current RPY, vector of desired RPY_VELOCITIES, and timestep
    ///         Outputs vector of desired RPY
    std::vector<double> rpy_integrator(std::vector<double> current_rpy, std::vector<double> desired_rpy_vel, int time_step);

    /// @brief  Function to differentiate wrist joint position
    ///         Takes in vector of desired wrist joint positions, current wrist joint positions, and timestep
    ///         Outputs vector of desired wrist joint velocities
    std::vector<double> joint_differentiator(
        std::vector<double> desired_wrist_joint_pos, std::vector<double> current_wrist_joint_pos, int time_step
    );
    
    /// @brief  Function for spm fk
    ///         Takes in a vector representing v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z, also takes in previous rpy
    ///         Outputs vector of RPY
    ///         Called by spm_fk()
    std::vector<double> v_to_rpy(std::vector<double> v, std::vector<double> prev_rpy);

    /// @brief  Function for spm ik
    ///         Takes in a vector of RPY
    ///         Outputs vector representing v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z
    ///         Called by spm_ik()
    std::vector<double> rpy_to_v(std::vector<double> rpy);

    /// @brief  Mathematics function for vector addition
    std::vector<double> vector_addition(std::vector<double> x, std::vector<double> y);

    /// @brief  Mathematics function for vector addition, for 3 vectors
    std::vector<double> vector_addition(std::vector<double> x, std::vector<double> y, std::vector<double> z);

    /// @brief  Mathematics function for vector scalar product 
    std::vector<double> vector_scalar_product(std::vector<double> v, double s);

    /// @brief  Mathematics function for dot product of vectors
    double vector_dot_product(std::vector<double> x, std::vector<double> y);

    /// @brief  Mathematics function for cross product of vectors of length 3
    std::vector<double> vector_cross_product(std::vector<double> x, std::vector<double> y);

    /// @brief  Mathematics function for finding a custom metric between euler configurations
    double euler_metric(std::vector<double> theta_a, std::vector<double> theta_b);
};