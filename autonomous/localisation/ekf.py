#!/usr/bin/python3
__package__ = "autonomous"
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

The Extended Kalman Filter (EKF) is a recursive
  statistical model for estimating a future state
  given a previous state and an approximate
  observation with associated covariances.
This was adapted for 2022 URC from previous code
  by Marcel Masque Salgado
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: None - utility class used by PoseConverter
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	Autonomous
AUTHOR(S):	Marcel Masque, Max Tory
CREATION:	07/05/2021
EDITED:		08/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Find accurate motion model covariance
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import math
import numpy as np


class Ekf:
    def __init__(self):
        self.dt = 0.0    # window of time to which a set of measurements applies

    def motion_model(self, x, u):
        """ Predict then next state of the system given the past state and a model
        This is identical to simple_motion_model, but expanded into its matrix components for clarity. Use
        simple_motion_model for real applications, as it is twice as fast
        :param x: [[x], [y]] (2x1),  # roll is unnecessary as the rover's forward direction is defined by pitch
                    and roll alone
        :param u: [[v], [pitch], [yaw]] (1x1)
        :return:
        """
        # Multiplied by state vector to get linear contributions to predicted state from past state
        F = np.array([[1.0, 0],
                      [0, 1.0]])

        # Multiplied by inputs vector to get nonlinear contributions of state and inputs to predicted state
        B = np.array([[self.dt * math.cos(u[2, 0]) * math.cos(u[1, 0]), 0, 0], # dx/v
                      [self.dt * math.sin(u[2, 0]) * math.cos(u[1, 0]), 0, 0]]) # dy/v
        x1 = np.matmul(F, x)    # keep position and yaw, but remove velocity.
        x2 = np.matmul(B, u)  # Get the change in x and y, and the change in yaw, as well as the new velocity.
        # updates the x, y and yaw according to the motion model
        return x1+x2

    def simple_motion_model(self, x, u):
        """ Predict then next state of the system given the past state and a model
        :param x: [[x], [y], [pitch], [yaw], [v]] (2x1),  # roll is unnecessary as the rover's forward direction is defined by pitch
                    and roll alone
        :param u: [[v, pitchrate, yawrate]] (1x1)
        :return:
        """
        retval = np.array([
            [x[0, 0] + math.cos(u[2, 0]) * u[0, 0] * math.cos(u[1, 0]) * self.dt],
            [x[1, 0] + math.sin(u[2, 0]) * u[0, 0] * math.cos(u[1, 0]) * self.dt]
        ]).astype(float)  # 2x1
        return retval
    
    def observation_gps(self, x):
        """
        Observations come in the form of x, y coordinates in a 2 x 5 numpy array
        This converts our predicted state into a predicted observation, to compare against our true observation
        x: 2x1 array [[x], [y], [pitch], [yaw], [v]]
        return: 2x1 array [[x], [y]]
        """
        return x

    def jacobF(self, x, u):
        """
        :param x: 2x1 array [[x], [y], [pitch], [yaw], [v]]
        :param u: 1x1 array [[v], [d_pitch/dt], [d_yaw/dt]]
        Jacobian of Motion Model
        motion model
        x_{t+1} = x_t+v*dt*cos(yaw)*cos(pitch)
        y_{t+1} = y_t+v*dt*sin(yaw)*cos(pitch)
        pitch_{t+1} = pitch_t+phi*dt
        yaw_{t+1} = yaw_t+omega*dt
        v_{t+1} = v{t}
        so
        dx/dpitch = -v*dt*cos(yaw)*sin(pitch)
        dx/dyaw = -v*dt*sin(yaw)*cos(pitch)
        dx/dv = dt*cos(yaw)*cos(pitch)
        dy/dpitch = -v*dt*sin(yaw)*sin(pitch)
        dy/dyaw = v*dt*cos(yaw)*cos(pitch)
        dy/dv = dt*sin(yaw)*cos(pitch)
        """
        jF = np.array([
            [1.0, 0.0],
            [0.0, 1.0]
            ])

        return jF.astype('float')

    def jacobH_gps(self):
        # Jacobian of Observation Model
        """
        x,y,pitch,yaw
        [1,0,0,0,0]
        [0,1,0,0,0]
        ])
        """
        return np.array([
            [1, 0],
            [0, 1]
        ]).astype(float)

    def predict(self, x_est, p_est, u, dt):
        """
        Predict new state and covariance based on past state, covariance and control inputs
        :param x_est: previous state estimate - (2x1)
        :param p_est: previous state covariance estimate - (5x5)
        :param u: control inputs (d_pitch/d_t, d_yaw/d_t, v) - (1x1)
        :param dt: time diff since last input
        :return: new predicted state and covariance - (2x1), (5x5)
        """
        self.dt = dt
        p_est = p_est.astype('float')
        # predicted state
        x_pred = self.simple_motion_model(x_est, u)  # - 2x1

        J_F = self.jacobF(x_est, u).astype('float')  # - 2x2

        Q = 0.3 * u[0] * dt * np.diag([math.cos(u[2]), math.sin(u[2])])  # predict state covariance
        Q = (Q ** 2).astype('float')

        p_pred = np.matmul(np.matmul(J_F, p_est), J_F.T) + Q  # J_F * p_est = 5x5 * J_F.T = 5x5

        return x_pred, p_pred  # 2x1, 5x5

    def correct_gps(self, x_pred, p_pred, z_obs, R):
        """
        Correct current model based on observation and its covariance
        :param x_pred: predicted x - (2x1)
        :param p_pred: prediction covariance - (2x2)
        :param z_obs: observation - (2x1)
        :param R: covariance of observation - (2x2)
        :return:
        """
        z_pred = self.observation_gps(x_pred).astype(float)  # 2x1

        J_H = self.jacobH_gps().astype(float)  # 2x5
        # J_H * p_pred = 2x5 * J_H.T = 2x2 + R = 2x2
        S = np.matmul(np.matmul(J_H, p_pred.astype(float)), J_H.T) + R.astype(float) # covariance of z_pred
        if np.linalg.det(S) == 0:
            return z_obs, R

        # numpy.linalg.inv can't always find the inverse of this matrix....? so we will do it ourselves
        det = 1 / (S[0, 0] * S[1, 1] - S[1, 0] * S[0, 1])  # please don't be 0
        s_inv = det * np.array([[S[1, 1], -S[0, 1]], [-S[1, 0], S[0, 0]]])

        # Difference between expected observation and true observation
        y = z_obs - z_pred  # 2x1 - 2x1 = 2x1

        # Kalman gain, scales by covariance of the observation measurement. High covariance -> K has a small determinant
        K = np.matmul(np.matmul(p_pred, J_H.T), s_inv)  # 5x5 * 2x5.T = 5x2 * 2x2 = 5x2

        # scale the expected - predicted obs by the Kalman gain - small Kalman gain means we don't trust the obs
        x_corrected = x_pred + np.matmul(K, y)  # 5x2 * 2x1 = 2x1 + 2x1 = 2x1
        x_corrected[2:3, 0] %= 2 * math.pi

        # formula for the covariance update, but factorised for simplicity.
        p_corrected = np.matmul((np.eye(len(x_corrected)) - np.matmul(K, J_H)), p_pred)  # (5x5 - (5x2 * 2x5)) * 5x5=5x5

        return x_corrected, p_corrected  # 2x1, 5x5

    def correct_imu(self, x_pred, p_pred, z_obs, R):
        """
        Correct current model based on observation and its covariance
        Not relevant until we have covariance of imu observations
        :param x_pred: predicted x - (2x1)
        :param p_pred: prediction covariance - (5x5)
        :param z_obs: observation - (2x1)
        :param R: covariance of observation (2x2)
        :return:
        """
        z_pred = self.observation_imu(x_pred)  # 2x1

        z_obs %= 2 * math.pi

        J_H = self.jacobH_imu()  # 2x5
        # 2x5 * 5x5 = 2x5 * 2x5.T = 2x2
        S = np.matmul(np.matmul(J_H, p_pred), J_H.T) + R # covariance of z_pred

        # Difference between expected observation and true observation
        y = z_obs - z_pred  # 2x1 - 2x1
        y %= 2 * math.pi

        # Kalman gain, scales by covariance of the observation measurement. High covariance -> K has a small determinant
        K = np.matmul(np.matmul(p_pred, J_H.T), np.linalg.inv(S))  # 5x5 * 2x5.T = 5x2 * 2x2 = 5x2

        # scale the expected - predicted obs by the Kalman gain - small Kalman gain means we don't trust the obs
        x_corrected = x_pred + np.matmul(K, y)  # 2x1 - (5x2 * 2x1) = 2x1
        x_corrected[2:3, 0] %= 2 * math.pi

        # formula for the covariance update, but factorised for simplicity.
        p_corrected = np.matmul((np.eye(len(x_corrected)) - np.matmul(K, J_H)), p_pred)  # (5x5 - (5x2 * 2x5)) * 5x5=5x5

        return x_corrected, p_corrected  # 2x1, 5x5
