#pragma once

#define _USE_MATH_DEFINES
#include <iostream>
#include <cmath>
#include "Eigen/Dense"

class SpmWrist {
    double deg2rad(double deg);
    double rad2deg(double rad);
    void initial_v(Eigen::Matrix<double, 3, 3> * v);
    void rotated_vector(double roll, double pitch, double yaw);
    double A(int index);
    double B(int index);
    double C(int index);
    void return_angles(void);
    double derive_angle(double A, double B, double C);

    double n[3] = {0, 2*M_PI/3, 4*M_PI/3};
    double alpha[3];
    double beta;
    double gamma;
    double angles[3];
    Eigen::Matrix<double, 3, 3> v_initial;
    Eigen::Matrix<double, 3, 3> v;

    public:
        double * motor_pos(double roll, double pitch, double yaw);
        SpmWrist();
};