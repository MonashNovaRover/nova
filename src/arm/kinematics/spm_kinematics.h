#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class performs the spm Pitch-Yaw-Roll (pyr) (strictly speaking, theta_x, theta_y, theta_z) integration, the
    spm IK, and the wrist joint angle differentiation.
It reads the resolver data, and desired Pitch-Yaw-Roll velocities, and
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
#define _USE_MATH_DEFINES
#include <cmath>

/* 
Class which solves the spm kinematics
*/
class SpmKinematics
{
    //------------------------------------------------------------//
    private:
   
    // Wrist geometry
    // JB: Move these to initialiser list so don't need cmath in this file?
    const double beta = 54.74 * M_PI / 180;
    const double gamma = 54.74 * M_PI / 180;
    const double alpha[3] = {M_PI/2, M_PI/2, 2*asin(sin(beta)*cos(M_PI/6))};
    const double eta[3] = {0.0, 2*M_PI/3, 4*M_PI/3};

    // Track internal state
    const int num_base_joints = 3;
    std::vector<double> current_pyr_pos (3);

    // Convenience maths functions
    // JB: Implemetn these as a library (like Print)

    /// @brief  Mathematics function for vector addition
    // JB: Overload + operator
    std::vector<double> vector_addition(std::vector<double> x, std::vector<double> y);

    /// @brief  Mathematics function for vector addition, for 3 vectors
    // JB: Don't need this
    std::vector<double> vector_addition(std::vector<double> x, std::vector<double> y, std::vector<double> z);

    /// @brief  Mathematics function for vector scalar product 
    // JB: Overload * operator
    std::vector<double> vector_scalar_product(std::vector<double> v, double s);

    /// @brief  Mathematics function for dot product of vectors
    // JB: Rename
    double vector_dot_product(std::vector<double> x, std::vector<double> y);

    /// @brief  Mathematics function for cross product of vectors of length 3
    // JB: Rename
    std::vector<double> vector_cross_product(std::vector<double> x, std::vector<double> y);

    /// @brief  Mathematics function for finding a custom metric between euler configurations
    double euler_metric(std::vector<double> theta_a, std::vector<double> theta_b);


    // Spm specific functions
    // JB: Doc styles - first line description
    /// @brief  Function for spm fk
    ///         Takes in a vector representing v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z, also takes in previous pyr
    ///         Outputs vector of pyr
    ///         Called by spm_fk()
    // JB: Why prev_pyr?
    std::vector<double> v_to_pyr(std::vector<double> v, std::vector<double> prev_pyr);

    /// @brief  Function for spm ik
    ///         Takes in a vector of pyr
    ///         Outputs vector representing v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z
    ///         Called by spm_ik()
    std::vector<double> pyr_to_v(std::vector<double> pyr);
    
    /// @brief  Function for spm fk, to solve the nonlinear system of 9 equations
    ///         Takes in vector of w = {w1x w1y w1z w2x w2y w2z w3x w3y w3z}, cos of alpha2, cos of alpha3, and a guess vector for v, and error margin
    ///         Outputs vector of v = {v1x v1y v1z v2x v2y v2z v3x v3y v3z}
    std::vector<double> spm_fk_system_solve(std::vector<double> w, double cos_a2, double cos_a3, std::vector<double> v_guess, double error_margin);

    /// @brief  Function to integrate pyr
    ///         Takes in vector of current pyr, vector of desired pyr_VELOCITIES, and timestep
    ///         Outputs vector of desired pyr
    std::vector<double> pyr_integrator(std::vector<double> current_pyr, std::vector<double> desired_pyr_vel, int time_step);

    /// @brief  Function to differentiate wrist joint position
    ///         Takes in vector of desired wrist joint positions, current wrist joint positions, and timestep
    ///         Outputs vector of desired wrist joint velocities
    std::vector<double> joint_differentiator(
        std::vector<double> desired_wrist_joint_pos, std::vector<double> current_wrist_joint_pos, int time_step
    );
    

    //------------------------------------------------------------//
    public:

    /// Constructor. Initialisers the solvers and starts the node
    SpmKinematics();

    // JB: Revisit comments
    /// @brief  Ultimate solver function for spm kinematics
    ///         Takes in current wrist joint positions (resolvers), and desired pyr velocities, and timestep
    ///         Outputs required wrist joint velocities
    ///         Or can be accessed in arm_kinematics.cpp
    ///         Calls numerous functions below in the process
    std::vector<double> spm_ik_velocity(std::vector<double> current_wrist_joint_pos, std::vector<double> desired_pyr_vel, int time_step);

    /// @brief  Function for spm fk
    ///         Takes in vector of wrist joint positions
    ///         Outputs vector of pyr
    ///         Calls v_to_pyr(), and spm_fk_system_solve() functions in the process
    std::vector<double> spm_fk(std::vector<double> wrist_joint_pos);

    /// @brief  Function for spm ik
    ///         Takes in vector of desired pyr
    ///         Outputs vector of required wrist joint positions
    ///         Calls pyr_to_v() function in the process
    std::vector<double> spm_ik(std::vector<double> desired_pyr_pos);
};