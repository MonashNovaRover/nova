#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class performs the spm Pitch-Yaw-Roll (pyr) (strictly speaking, theta_x, theta_y, theta_z) integration, the
    spm IK, and the wrist joint angle differentiation, in order to solve for Velocity IK.
The Velocity IK takes in current wrist joint positions, and desired Pitch-Yaw-Roll velocities, and
    outputs the desired wrist joint velocities.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S):   Alexander Li
CREATION:	 23/04/2022
EDITED:		 30/04/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
    - convert vectors into Eigen::Matrix where appropriate
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
 
#include <vector>
#include <chrono>
#include <Eigen/Dense>
#define _USE_MATH_DEFINES
#include <cmath>

/* 
Class which solves the spm kinematics
*/
class SpmKinematics
{
    //------------------------------------------------------------//
    private:
   
    // Define wrist geometry
    const double beta = 54.74 * M_PI / 180;
    const double gamma = 54.74 * M_PI / 180;
    const double alpha[3] = {M_PI/2, M_PI/2, 2*asin(sin(beta)*cos(M_PI/6))};
    const double eta[3] = {0.0, 2*M_PI/3, 4*M_PI/3};

    // Track internal state
    const int num_base_joints = 3;
    std::vector<double> current_pyr_pos {0, 0, 0};
    std::chrono::time_point<std::chrono::steady_clock> time_at_previous_call; 


    /// @brief  Custom metric between euler configurations
    double euler_metric(const std::vector<double> &theta_a, const std::vector<double> &theta_b);

    // Spm specific functions
    /// @brief  Converts v vectors into Pitch Yaw Roll angles
    ///         Takes in a vector representing v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z, also takes in previous pyr
    ///         Outputs vector of pyr
    std::vector<double> v_to_pyr(const std::vector<double> &v, const std::vector<double> &prev_pyr);

    /// @brief  Converts Pitch Yaw Roll angles to v vectors
    ///         Takes in a vector of pyr
    ///         Outputs vector representing v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z
    std::vector<double> pyr_to_v(const std::vector<double> &pyr);
    
    /// @brief  Solves the nonlinear system of 9 equations specific to spm
    ///         Takes in vector of w = {w1x w1y w1z w2x w2y w2z w3x w3y w3z}, cos of alpha2, cos of alpha3, and a guess vector for v, and error margin
    ///         Outputs vector of v = {v1x v1y v1z v2x v2y v2z v3x v3y v3z}
    std::vector<double> spm_fk_system_solve(
        const std::vector<double> &w, const double cos_a2, const double cos_a3, const std::vector<double> &v_guess, const double error_margin
    );

    /// @brief  Integrates Pitch Yaw Roll positions over time
    ///         Takes in vector of current pyr, vector of desired pyr_VELOCITIES, and timestep
    ///         Outputs vector of desired pyr
    std::vector<double> pyr_integrator(const std::vector<double> &current_pyr, const std::vector<double> &desired_pyr_vel, double time_step);

    /// @brief  Differentiates wrist joint positions with respect to time
    ///         Takes in vector of desired wrist joint positions, current wrist joint positions, and timestep
    ///         Outputs vector of desired wrist joint velocities
    std::vector<double> joint_differentiator(
        const std::vector<double> &desired_wrist_joint_pos, const std::vector<double> &current_wrist_joint_pos, double time_step
    );
    

    //------------------------------------------------------------//
    public:

    /// Constructor
    SpmKinematics();

    /// @brief  Velocity IK solver for SPM
    ///         Takes in current wrist joint positions (resolvers), and desired pyr velocities, and timestep
    ///         Outputs required wrist joint velocities
    std::vector<double> spm_ik_velocity(
        const std::vector<double> &current_wrist_joint_pos, const std::vector<double> &desired_pyr_vel
    );

    /// @brief  Performs SPM FK
    ///         Takes in vector of wrist joint positions
    ///         Outputs vector of pyr
    std::vector<double> spm_fk(const std::vector<double> &wrist_joint_pos);

    /// @brief  Performs SPM IK
    ///         Takes in vector of desired pyr
    ///         Outputs vector of required wrist joint positions
    std::vector<double> spm_ik(const std::vector<double> &desired_pyr_pos);
};