/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Alexander Li
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "spm_kinematics.h"

#include <Eigen/Dense>

//------------------------------------------------------------------------
// Private functions

// custom metric for euler angles
double euler_metric(const std::vector<double> &theta_a, const std::vector<double> &theta_b)
{ 
    double dist_x = abs(theta_a[0] - theta_b[0]);
    if (dist_x > M_PI) {dist_x = 2*M_PI - dist_x;}
    double dist_y = abs(theta_a[1] - theta_b[1]);
    if (dist_y > M_PI) {dist_y = 2*M_PI - dist_y;}
    double dist_z = abs(theta_a[2] - theta_b[2]);
    if (dist_z > M_PI) {dist_z = 2*M_PI - dist_z;}
    
    return sqrt(dist_x * dist_x + dist_y * dist_y + dist_z * dist_z);
}

// Spm specific functions
// convert v vectors to pyr
std::vector<double> SpmKinematics::v_to_pyr(const std::vector<double> &v, const std::vector<double> &prev_pyr)
{
    // extract v vectors
    // JB: Seems like a waste of time. Implement a different input parameter?
    Eigen::Vector3d v1(v[0], v[1], v[2]);
    Eigen::Vector3d v2(v[3], v[4], v[5]);
    Eigen::Vector3d v3(v[6], v[7], v[8]);

    // convert to axes of output frame
    Eigen::Vector3d z = (v1 + v2 + v3) / (v1 + v2 + v3).norm();
    Eigen::Vector3d x = (v2 - cos(beta)*z) / sin(beta);
    Eigen::Vector3d y = z.cross(x);

    // set up solutions
    std::vector<std::vector<double>> pyr_solutions (2);
    pyr_solutions.push_back(std::vector<double> (3));
    pyr_solutions.push_back(std::vector<double> (3));

    // Find 2 possible theta_y
    pyr_solutions[0][1] = asin(z[0]);
    pyr_solutions[1][1] = M_PI - pyr_solutions[0][1];
    for (unsigned int i = 0; i < pyr_solutions.size(); i++){
        // Find corresponding angles of theta_x
        pyr_solutions[i][0] = atan2(-z[1]/cos(pyr_solutions[i][1]), z[2]/cos(pyr_solutions[i][1]));
        // Find corresponding angles of theta_z
        pyr_solutions[i][2] = atan2(-y[0]/cos(pyr_solutions[i][1]), x[0]/cos(pyr_solutions[i][1]));
    }
    // Choose between the solutions; select the one with smaller metric to previous Euler configuration
    if (euler_metric(pyr_solutions[0], prev_pyr) < euler_metric(pyr_solutions[1], prev_pyr)) {
        return pyr_solutions[0];
    }
    return pyr_solutions[1];
}

// convert pyr to v vectors
std::vector<double> SpmKinematics::pyr_to_v(const std::vector<double> &pyr)
{
    double theta_x = pyr[0];
    double theta_y = pyr[1];
    double theta_z = pyr[2];
    
    // convert into the axes of output frame
    // JB: Good place for an Eigen Matrix?
    Eigen::Vector3d x (
        cos(theta_z) * cos(theta_y),
        cos(theta_z) * sin(theta_y) * sin(theta_x) + sin(theta_z) * cos(theta_x),
        -cos(theta_z) * sin(theta_y) * cos(theta_x) + sin(theta_z) * sin(theta_x)
    );
    Eigen::Vector3d y (
        -sin(theta_z) * cos(theta_y),
        -sin(theta_z) * sin(theta_y) * sin(theta_x) + cos(theta_z) * cos(theta_x), 
        sin(theta_z) * sin(theta_y) * cos(theta_x) + cos(theta_z) * sin(theta_x)
    );
    Eigen::Vector3d z (
        sin(theta_y),
        -cos(theta_y) * sin(theta_x),
        cos(theta_y) * cos(theta_x)
    );

    // find v2
    Eigen::Vector3d v2 = sin(beta) * x + cos(beta) * z;

    // find p1, p3
    Eigen::Vector3d p1 = cos(2 *M_PI/3) * x + sin(2 *M_PI/3) * y;
    Eigen::Vector3d p3 = cos(-2 *M_PI/3) * x + sin(-2 *M_PI/3) * y;

    //find v1, v3
    Eigen::Vector3d v1 = sin(beta) * p1 + cos(beta) * z;
    Eigen::Vector3d v3 = sin(beta) * p3 + cos(beta) * z;

    //return v vectors as a single array
    // JB: Can we not
    // AL: I'll have a think.
    std::vector<double> v = {v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], v3[0], v3[1], v3[2]};
    return v;
}

// solve nonlinear system of equations F(v) = 0 from fk
std::vector<double> SpmKinematics::spm_fk_system_solve(
    const std::vector<double> &w, const double cos_a2, const double cos_a3, const std::vector<double> &v_guess, const double error_margin
)
{
    // Create vector for initial guess
    Eigen::Matrix<double, 9, 1> v (v_guess.data());
    // Create first Function output
    Eigen::Matrix<double, 9, 1> F;
    F << w[0]*v[0] + w[1]*v[1] + w[2]*v[2] - cos_a2,
         w[3]*v[3] + w[4]*v[4] + w[5]*v[5] - cos_a2,
         w[6]*v[6] + w[7]*v[7] + w[8]*v[8] - cos_a2,
         v[0]*v[3] + v[1]*v[4] + v[2]*v[5] - cos_a3,
         v[0]*v[6] + v[1]*v[7] + v[2]*v[8] - cos_a3,
         v[3]*v[6] + v[4]*v[7] + v[5]*v[8] - cos_a3,
         v[0]*v[0] + v[1]*v[1] + v[2]*v[2] - 1,
         v[3]*v[3] + v[4]*v[4] + v[5]*v[5] - 1,
         v[6]*v[6] + v[7]*v[7] + v[8]*v[8] - 1;

    // Main loop of solver
    while (F.norm() > error_margin) {
        // Find Jacobian for previous v
        
        Eigen::Matrix<double, 9, 9> J;
        J << w[0], w[1], w[2], 0, 0, 0, 0, 0, 0,
             0, 0, 0, w[3], w[4], w[5], 0, 0, 0,
             0, 0, 0, 0, 0, 0, w[6], w[7], w[8],
             v[3], v[4], v[5], v[0], v[1], v[2], 0, 0, 0,
             v[6], v[7], v[8], 0, 0, 0, v[0], v[1], v[2],
             0, 0, 0, v[6], v[7], v[8], v[3], v[4], v[5],
             2*v[0], 2*v[1], 2*v[2], 0, 0, 0, 0, 0, 0,
             0, 0, 0, 2*v[3], 2*v[4], 2*v[5], 0, 0, 0,
             0, 0, 0, 0, 0, 0, 2*v[6], 2*v[7], 2*v[8];

        // Find step to get next v
        Eigen::Matrix<double, 9, 1> v_step;
        v_step = J.colPivHouseholderQr().solve(-F);
        // Find new v
        v += v_step;
        // Find new function output
        F << w[0]*v[0] + w[1]*v[1] + w[2]*v[2] - cos_a2,
             w[3]*v[3] + w[4]*v[4] + w[5]*v[5] - cos_a2,
             w[6]*v[6] + w[7]*v[7] + w[8]*v[8] - cos_a2,
             v[0]*v[3] + v[1]*v[4] + v[2]*v[5] - cos_a3,
             v[0]*v[6] + v[1]*v[7] + v[2]*v[8] - cos_a3,
             v[3]*v[6] + v[4]*v[7] + v[5]*v[8] - cos_a3,
             v[0]*v[0] + v[1]*v[1] + v[2]*v[2] - 1,
             v[3]*v[3] + v[4]*v[4] + v[5]*v[5] - 1,
             v[6]*v[6] + v[7]*v[7] + v[8]*v[8] - 1;
    }

    //Create output vector
    return std::vector<double> {v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8]};
}


// integrate pyr vel into pyr pos
std::vector<double> SpmKinematics::pyr_integrator(const std::vector<double> &current_pyr, const std::vector<double> &desired_pyr_vel, int time_step)
{
    // JB: See if can track the timestep internally (but not in this function, since is used)
    // JB: Requires the calling function calls this at a consistent rate
    std::vector<double> pyr_pos (3);
    for (unsigned int i = 0; i < 3; i++) {
        if (desired_pyr_vel[i] != 0){
            pyr_pos[i] = current_pyr[i] + desired_pyr_vel[i] * time_step;
        }
    }

    return pyr_pos;
}

// differentiate desired joint angles into desired joint velocities
std::vector<double> SpmKinematics::joint_differentiator(
    const std::vector<double> &desired_wrist_joint_pos, const std::vector<double> &current_wrist_joint_pos, int time_step
)
{
    std::vector<double> joint_vel (3);
    for (unsigned int i = 0; i < 3; i++) {
        joint_vel[i] = (desired_wrist_joint_pos[i] - current_wrist_joint_pos[i]) / time_step;
    }
    return joint_vel;
}

//-------------------------------------------------------------------------------------
// Public functions

// Main solver
std::vector<double> SpmKinematics::spm_ik_velocity(const std::vector<double> &current_wrist_joint_pos, const std::vector<double> &desired_pyr_vel, int time_step)
{
    // Update the current pyr position
    current_pyr_pos = spm_fk(current_wrist_joint_pos);
    // integrate
    std::vector<double> desired_pyr_pos = pyr_integrator(current_pyr_pos, desired_pyr_vel, time_step);
    // ik
    std::vector<double> desired_wrist_joint_pos = spm_ik(desired_pyr_pos);
    // differentiate
    std::vector<double> desired_wrist_joint_vel = joint_differentiator(current_wrist_joint_pos, desired_wrist_joint_pos, time_step);

    return desired_wrist_joint_vel;
}

//perform spm position fk
std::vector<double> SpmKinematics::spm_fk(const std::vector<double> &wrist_joint_pos)
{
    //TODO: implement simultaneous equation solver
    // JB: Check the output of pyr_to_v(std::vector<double> (3)) matches signs of v_guess below
    // std::vector<double> v_guess = {-1, 1, 1, 1, 1, 1, -1, -1, 1};
    // Use the last pyr position to get a guess for the FK solver
    std::vector<double> v_guess = pyr_to_v(current_pyr_pos);
    std::vector<double> theta = wrist_joint_pos;

    // solve fk, obtain v1, v2, v3, store into an array of v[9] = {v1x v1y v1z v2x v2y v2z v3x v3y v3z}
    // find w1 w2 w3 as w[9] = {w1x w1y w1z w2x w2y w2z w3x w3y w3z}
    std::vector<double> w (9);
    for (unsigned int i = 0; i < 3; i++) {
        //w_ix
        w[3*i+0] = sin(eta[i])*sin(gamma)*cos(alpha[0]) - (cos(eta[i])*sin(theta[i])-sin(eta[i])*cos(gamma)*cos(theta[i]))*sin(alpha[i]);
        //w_iy
        w[3*i+1] = cos(eta[i])*sin(gamma)*cos(alpha[0]) + (sin(eta[i])*sin(theta[i])+cos(eta[i])*cos(gamma)*cos(theta[i]))*sin(alpha[i]);
        //w_iz
        w[3*i+2] = -cos(gamma)*cos(alpha[0]) + sin(gamma)*cos(theta[i])*sin(alpha[i]);
    }

    // solve system of nonlinear equations
    std::vector<double> v = spm_fk_system_solve(w, cos(alpha[1]), cos(alpha[2]), v_guess, 0.0001);

    // JB: Use IK to verify the FK solution? Might not be needed

    //convert v vectors to pyr
    return v_to_pyr(v, current_pyr_pos);
}

// perform spm position ik
std::vector<double> SpmKinematics::spm_ik(const std::vector<double> &desired_pyr_pos)
{
    // initialise output
    std::vector<double> joint_angles (3);
    // convert pyr to v
    std::vector<double> v = pyr_to_v(desired_pyr_pos);

    // solve for joint angles
    for (unsigned int i = 0; i < 3; i++) {
        double A = (-sin(eta[i])*sin(gamma)*cos(alpha[i])+sin(eta[i])*cos(gamma)*sin(alpha[i])) * (-v[3*i+0])
            + (cos(eta[i])*sin(gamma)*cos(alpha[i])-cos(eta[i])*cos(gamma)*sin(alpha[i])) * v[3*i+1]
            + (cos(gamma)*cos(alpha[i])-sin(gamma)*sin(alpha[i])) * v[3*i+2] - cos(alpha[1]);
        double B = 2 * (cos(eta[i])*sin(alpha[i]) * (-v[3*i+0]) + sin(eta[i])*sin(alpha[i]) * v[3*i+1]);
        double C = (-sin(eta[i])*sin(gamma)*cos(alpha[i])-sin(eta[i])*cos(gamma)*sin(alpha[i])) * (-v[3*i+0])
            + (cos(eta[i])*sin(gamma)*cos(alpha[i])+cos(eta[i])*cos(gamma)*sin(alpha[i])) * v[3*i+1]
            + (-cos(gamma)*cos(alpha[i])+sin(gamma)*sin(alpha[i])) * v[3*i+2] - cos(alpha[1]);
        // take positive root
        double T = (-B + sqrt(B*B - 4*A*C)) / (2*A);
        joint_angles[i] = 2 * atan(T);
    }

    return joint_angles;
}
