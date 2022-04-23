#include "spm.hpp"

SpmWrist::SpmWrist(void) {
    
    /*
    Values recorded 13/02/22
    These are physical measurements of the SPM wrist
    */
    beta = deg2rad(54.74);
    gamma = deg2rad(54.74);
    alpha[0] = M_PI/2.0;
    alpha[1] = M_PI/2.0;
    alpha[2] = 2.0*asin(sin(beta)*cos(M_PI/6.0));

    // Vector state and zero-state
    Eigen::Matrix<double, 3, 3> v;
    Eigen::Matrix<double, 3, 3> v_initial;
    I
    initial_v(&v);
    initial_v(&v_initial);
}

double SpmWrist::deg2rad(double deg) {
    // Standard degrees to radians
    return deg * M_PI / 180.0;
}

double SpmWrist::rad2deg(double rad) {
    // Standard radians to degrees
    return rad * 180.0 / M_PI;
}

void SpmWrist::initial_v(Eigen::Matrix<double, 3, 3> * v) {
    // Initialises the zero position vector
    for (int i=0; i<3; i++) {
        (*v)(i,0) = -sin(n[i])*sin(beta);
        (*v)(i,1) =  cos(n[i])*sin(beta);
        (*v)(i,2) =  cos(beta); 
    }
}

void SpmWrist::rotated_vector(double roll, double pitch, double yaw) {
    // Rotates each position vector by the require roll, pitch, yaw

    Eigen::Matrix<double, 3, 3> R;
    R << cos(roll), -sin(roll), 0.0,
         sin(roll), cos(roll), 0.0,
         0.0, 0.0, 1.0;
    
    Eigen::Matrix<double, 3, 3> P;
    P << cos(pitch), 0.0, sin(pitch),
         0.0, 1.0, 0.0,
         -sin(pitch), 0, cos(pitch);
    
    Eigen::Matrix<double, 3, 3> Y;
    Y << 1.0, 0.0, 0.0,
         0.0, cos(yaw), -sin(yaw),
         0.0, sin(yaw), cos(yaw);

    // Apply rotations to each vector
    for (int row=0; row<3; row++) {
        v.row(row) = R * v_initial.row(row).transpose();
        v.row(row) = P * v.row(row).transpose();
        v.row(row) = Y * v.row(row).transpose();
    }
}

/* --------------------------------------- */
// These functions are a representation of an analytical solution
// A*T^2 + 2*B*T + C = 0

double SpmWrist::A(int index) {
    return  -v(index, 0) * (-sin(n[index])*sin(gamma)*cos(alpha[index]) + sin(n[index])*cos(gamma)*sin(alpha[index]))
           + v(index, 1) * (cos(n[index])*sin(gamma)*cos(alpha[index]) - cos(n[index])*cos(gamma)*sin(alpha[index]))
           + v(index, 2) * (cos(gamma)*cos(alpha[index]) - sin(gamma)*sin(alpha[index]))
           - cos(alpha[1]);
}

double SpmWrist::B(int index) {
    return   -v(index, 0)*cos(n[index])*sin(alpha[index])
            + v(index, 1)*sin(n[index])*sin(alpha[index]);
}

double SpmWrist::C(int index) {
    return   -v(index, 0)*(-sin(n[index])*sin(gamma)*cos(alpha[index]) - sin(n[index])*cos(gamma)*sin(alpha[index]))
            + v(index, 1)*(cos(n[index])*sin(gamma)*cos(alpha[index]) + cos(n[index])*cos(gamma)*sin(alpha[index]))
            + v(index, 2)*(-cos(gamma)*cos(alpha[index]) + sin(gamma)*sin(alpha[index]))
            - cos(alpha[1]);
}

double SpmWrist::derive_angle(double A, double B, double C) {
    // Solve both the quadratic solutions and then take the positive angles
    double T_pos = (-2.0*B + sqrt(4.0*pow(B,2) - 4.0*A*C))/(2*A);
    double T_neg = (-2.0*B - sqrt(4.0*pow(B,2) - 4.0*A*C))/(2*A);
    double theta_pos = rad2deg(2.0*atan(T_pos));
    double theta_neg = rad2deg(2.0*atan(T_neg));
    return (theta_pos > theta_neg) ? theta_pos : theta_neg;
}
/* --------------------------------------- */

void SpmWrist::return_angles(void) {
    // Generate all quadratic coefficients

    double a, b, c;
    for (int vector = 0; vector < 3; vector++) {
      a = A(vector);
      b = B(vector);
      c = C(vector);
      angles[vector] = derive_angle(a, b, c);
    }
}

double * SpmWrist::motor_pos(double roll, double pitch, double yaw) {
    // Return the angles required for each motor
    // !TODO: return a vec
 
    rotated_vector(roll, pitch, yaw);
    return_angles();
    return angles;
}

int main(void) {
    SpmWrist wrist;

    double * motors_origin = wrist.motor_pos(0.0, 0.0, 0.0);
    
    for (int i=0; i<3; i++) {
        std::cout << motors_origin[i] << " ";
    }
    std::cout << "\n" << std::endl;

    double * motors = wrist.motor_pos(0.0, 0.0, M_PI/2.0);
    
    for (int i=0; i<3; i++) {
        std::cout << motors[i] << " ";
    }
    std::cout << "\n" << std::endl;
    return 0;
}