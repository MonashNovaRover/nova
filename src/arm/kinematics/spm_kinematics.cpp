/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Alexander Li
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "spm_kinematics.h"
#include <cmath>
#include <Eigen/Core>
#define _USE_MATH_DEFINES

//------------------------------------------------------------------------
// Private functions

// Convenience maths functions
// vector addition for 2 vectors
std::vector<double> SpmKinematics::vector_addition(std::vector<double> x, std::vector<double> y)
{
    std::vector<double> output = {0, 0, 0};
    for (unsigned int i = 0; i < x.size(); i++) {
        output[i] = x[i] + y[i];
    }
    return output;
}

// vector addition for 3 vectors
std::vector<double> SpmKinematics::vector_addition(std::vector<double> x, std::vector<double> y, std::vector<double> z)
{
    std::vector<double> output = {0, 0, 0};
    for (unsigned int i = 0; i < x.size(); i++) {
        output[i] = x[i] + y[i] + z[i];
    }
    return output;
}

// vector scalar product
std::vector<double> SpmKinematics::vector_scalar_product(std::vector<double> v, double s)
{
    std::vector<double> output = {0, 0, 0};
    for (unsigned int i = 0; i < v.size(); i++) {
        output[i] = v[i] * s;
    }
    return output;
}

//  dot product of vectors
double SpmKinematics::vector_dot_product(std::vector<double> x, std::vector<double> y)
{
    double output = 0;
    for (unsigned int i = 0; i < x.size(); i++) {
        output += x[i] * y[i];
    }
    return output;
}

// cross product of vectors of length 3
std::vector<double> SpmKinematics::vector_cross_product(std::vector<double> x, std::vector<double> y)
{
    std::vector<double> z = {0, 0, 0};
    z[0] = x[1] * y[2] - x[2] * y[1];
    z[1] = x[2] * y[0] - x[0] * y[2];
    z[2] = x[0] * y[1] - x[1] * y[0];
    return z;
}

// custom metric for euler angles
double euler_metric(std::vector<double> theta_a, std::vector<double> theta_b)
{
    double dist_x = abs(theta_a[0] - theta_b[0]);
    if (dist_x > 180.0) {dist_x -= 180;}
    double dist_y = abs(theta_a[1] - theta_b[1]);
    if (dist_y > 180.0) {dist_y -= 180;}
    double dist_z = abs(theta_a[2] - theta_b[2]);
    if (dist_z > 180.0) {dist_z -= 180;}
    
    return sqrt(dist_x * dist_x + dist_y * dist_y + dist_z * dist_z);
}

// Spm specific functions
// convert v vectors to rpy
std::vector<double> SpmKinematics::v_to_rpy(std::vector<double> v, std::vector<double> prev_rpy)
{
    // extract v vectors
    std::vector<double> v1 = {v[0], v[1], v[2]};
    std::vector<double> v2 = {v[3], v[4], v[5]};
    std::vector<double> v3 = {v[6], v[7], v[8]};

    // convert to axes of output frame
    std::vector<double> z = vector_scalar_product(
        vector_addition(v1, v2, v3), sqrt(vector_dot_product(vector_addition(v1, v2, v3), vector_addition(v1, v2, v3)))
        );
    std::vector<double> x = vector_scalar_product(vector_addition(v2, vector_scalar_product(z, -cos(beta))), 1/sin(beta));
    std::vector<double> y = vector_cross_product(z, x);

    // set up solutions
    std::vector<double> theta_a = {0, 0, 0};
    std::vector<double> theta_b = {0, 0, 0};
    // finding 2 possible theta_y
    theta_a[1] = asin(z[0]);
    theta_b[1] = M_PI - theta_a[1];
    // finding corresponding angles of theta_x
    theta_a[0] = atan2(-z[1]/cos(theta_a[1]), z[2]/cos(theta_a[1]));
    theta_b[0] = atan2(-z[1]/cos(theta_b[1]), z[2]/cos(theta_b[1]));
    // finding corresponding angles of theta_z
    theta_a[2] = atan2(-y[0]/cos(theta_a[1]), x[0]/cos(theta_a[1]));
    theta_b[2] = atan2(-y[0]/cos(theta_b[1]), x[0]/cos(theta_b[1]));
    // choose between the solutions; select the one with smaller metric to previous Euler configuration
    std::vector<double> rpy = {0, 0, 0};
    if (euler_metric(theta_a, prev_rpy) < euler_metric(theta_b, prev_rpy)) {
        rpy[0] = theta_a[0];
        rpy[1] = theta_a[1];
        rpy[2] = theta_a[2];
    } else {
        rpy[0] = theta_b[0];
        rpy[1] = theta_b[1];
        rpy[2] = theta_b[2];
    }
    return rpy;
}

// convert rpy to v vectors
std::vector<double> SpmKinematics::rpy_to_v(std::vector<double> rpy)
{
    double theta_x = rpy[0];
    double theta_y = rpy[1];
    double theta_z = rpy[2];
    
    // convert into the axes of output frame
    std::vector<double> x = {
        cos(theta_z) * cos(theta_y),
        cos(theta_z) * sin(theta_y) * sin(theta_x) + sin(theta_z) * cos(theta_x),
        -cos(theta_z) * sin(theta_y) * cos(theta_x) + sin(theta_z) * sin(theta_x)
    };
    std::vector<double> y = {
        -sin(theta_z) * cos(theta_y),
        -sin(theta_z) * sin(theta_y) * sin(theta_x) + cos(theta_z) * cos(theta_x), 
        sin(theta_z) * sin(theta_y) * cos(theta_x) + cos(theta_z) * sin(theta_x)
    };
    std::vector<double> z = {
        sin(theta_y),
        -cos(theta_y) * sin(theta_x),
        cos(theta_y) * cos(theta_x)
    };

    // find v2
    std::vector<double> v2 = vector_addition(vector_scalar_product(x, sin(beta)), vector_scalar_product(z, cos(beta)));

    // find p1, p3
    std::vector<double> p1 = vector_addition(vector_scalar_product(x, cos(2 *M_PI/3)), vector_scalar_product(y, sin(2 *M_PI/3)));
    std::vector<double> p3 = vector_addition(vector_scalar_product(x, cos(-2 *M_PI/3)), vector_scalar_product(y, sin(-2 *M_PI/3)));

    //find v1, v3
    std::vector<double> v1 = vector_addition(vector_scalar_product(p1, sin(beta)), vector_scalar_product(z, cos(beta)));
    std::vector<double> v3 = vector_addition(vector_scalar_product(p3, sin(beta)), vector_scalar_product(z, cos(beta)));

    //return v vectors as a single array
    std::vector<double> v = {v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], v3[0], v3[1], v3[2]};
    return v;
}

// solve nonlinear system of equations from fk
std::vector<double> SpmKinematics::fk_system_solve(std::vector<double> w, std::vector<double> v_guess)
{

}


// integrate rpy vel into rpy pos
std::vector<double> SpmKinematics::rpy_integrator(std::vector<double> current_rpy, std::vector<double> desired_rpy_vel, int time_step)
{
    std::vector<double> rpy_pos = {0, 0, 0};
    for (unsigned int i = 0; i < 3; i++) {
        rpy_pos[i] = current_rpy[i] + desired_rpy_vel[i] * time_step;
    }

    return rpy_pos;
}

// differentiate desired joint angles into desired joint velocities
std::vector<double> SpmKinematics::joint_differentiator(
        std::vector<double> desired_wrist_joint_pos, std::vector<double> current_wrist_joint_pos, int time_step
    )
{
    std::vector<double> joint_vel = {0, 0, 0};
    for (unsigned int i = 0; i < 3; i++) {
        joint_vel[i] = (desired_wrist_joint_pos[i] - current_wrist_joint_pos[i]) / time_step;
    }
    return joint_vel;
}

//-------------------------------------------------------------------------------------
// Public functions
// Constructor
SpmKinematics::SpmKinematics() {}

// Main solver
std::vector<double> SpmKinematics::solve(std::vector<double> current_wrist_joint_pos, std::vector<double> desired_rpy_vel, int time_step)
{
    // obtain current rpy
    previous_rpy_pos = current_rpy_pos;
    current_rpy_pos = spm_fk(current_wrist_joint_pos);
    // integrate
    std::vector<double> desired_rpy_pos = rpy_integrator(current_rpy_pos, desired_rpy_vel, time_step);
    // ik
    std::vector<double> desired_wrist_joint_pos = spm_ik(desired_rpy_pos);
    // differentiate
    std::vector<double> desired_wrist_joint_vel = joint_differentiator(current_wrist_joint_pos, desired_wrist_joint_pos, time_step);

    return desired_wrist_joint_vel;
}

//perform spm position fk
std::vector<double> SpmKinematics::spm_fk(std::vector<double> current_wrist_joint_pos)
{
    //TODO: implement simultaneous equation solver
    std::vector<double> v_guess = {-1, 1, 1, 1, 1, 1, -1, -1, 1};
    std::vector<double> theta = {current_wrist_joint_pos[0], current_wrist_joint_pos[1], current_wrist_joint_pos[2]};

    //solve fk, obtain v1, v2, v3, store into an array of v[9] = {v1x v1y v1z v2x v2y v2z v3x v3y v3z}
    // find w1 w2 w3 as w[9] = {w1x w1y w1z w2x w2y w2z w3x w3y w3z}
    std::vector<double> w = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    for (unsigned int i = 0; i < 3; i++) {
        //w_ix
        w[3*i+0] = sin(eta[i])*sin(gamma)*cos(alpha[0]) - (cos(eta[i])*sin(theta[i])-sin(eta[i])*cos(gamma)*cos(theta[i]))*sin(alpha[i]);
        //w_iy
        w[3*i+1] = cos(eta[i])*sin(gamma)*cos(alpha[0]) + (sin(eta[i])*sin(theta[i])+cos(eta[i])*cos(gamma)*cos(theta[i]))*sin(alpha[i]);
        //w_iz
        w[3*i+2] = -cos(gamma)*cos(alpha[0]) + sin(gamma)*cos(theta[i])*sin(alpha[i]);
    }

    // solve system of nonlinear equations
    std::vector<double> v = fk_system_solve(w, v_guess);

    //convert v vectors to rpy
    return v_to_rpy(v, previous_rpy_pos);
}

// perform spm position ik
std::vector<double> SpmKinematics::spm_ik(std::vector<double> desired_rpy_pos)
{
    std::vector<double> theta = {0, 0, 0};
    // convert rpy to v
    std::vector<double> v = rpy_to_v(desired_rpy_pos);

    // solve for theta
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
        theta[i] = 2 * atan(T);
    }

    return theta;
}

