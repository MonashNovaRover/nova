#!/usr/bin/python3
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
EDITED:		07/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import math
import numpy as np
import matplotlib.pyplot as plt


class Ekf:
    def __init__(self):

        self.Q = np.diag([
            0.3,  # variance of location on x-axis | this is really the SD, and same for below, no?
            0.3,  # variance of location on y-axis
            0,#np.deg2rad(25.0),  # variance of yaw angle
            0.5  # variance of velocity
        ]) ** 2  # predict state covariance

        self.dt = 0.0    # window of time to which a set of measurements applies

    def motion_model(self, x, u):
        """ Predict then next state of the system given the past state and a model
        This is identical to simple_motion_model, but expanded into its matrix components for clarity. Use
        simple_motion_model for real applications, as it is twice as fast
        :param x: [[x], [y], [pitch], [yaw], [v]],  # roll is unnecessary as the rover's forward direction is defined by pitch
                    and roll alone
        :param u: [[v, pitchrate, yawrate]]
        :return:
        """
        # Multiplied by state vector to get linear contributions to predicted state from past state
        F = np.array([[1.0, 0, 0, 0, 0],
                      [0, 1.0, 0, 0, 0],
                      [0, 0, 1.0, 0, 0],
                      [0, 0, 0, 1.0, 0],
                      [0, 0, 0, 0, 0]])

        # Multiplied by inputs vector to get nonlinear contributions of state and inputs to predicted state
        B = np.array([[self.dt * math.cos(x[3, 0]) * math.cos(x[2, 0]), 0, 0], # dx/v
                      [self.dt * math.sin(x[3, 0]) * math.cos(x[2, 0]), 0, 0], # dy/v
                      [0.0, self.dt, 0],   # time interval
                      [0.0, 0.0, self.dt],   # time interval
                      [1.0, 0.0, 0.0]])  # to set state velocity to input velocity (can account for accel later)
        x1 = np.matmul(F, x)    # keep position and yaw, but remove velocity. 
        x2 = np.matmul(B, u.T)  # Get the change in x and y, and the change in yaw, as well as the new velocity.
        # updates the x, y and yaw according to the motion model
        return x1+x2

    def simple_motion_model(self, x, u):
        """ Predict then next state of the system given the past state and a model
        :param x: [[x], [y], [pitch], [yaw], [v]],  # roll is unnecessary as the rover's forward direction is defined by pitch
                    and roll alone
        :param u: [[v, pitchrate, yawrate]]
        :return:
        """
        retval = np.array([
            [x[0, 0] + math.cos(x[3, 0]) * u[0, 0] * math.cos(x[2, 0]) * self.dt],
            [x[1, 0] + math.sin(x[3, 0]) * u[0, 0] * math.cos(x[2, 0]) * self.dt],
            [x[2, 0] + u[0, 1] * self.dt],
            [x[3, 0] + u[0, 2] * self.dt],
            [u[0, 0]]
        ])
        return retval
    
    def observation_model(self, x):
        """
        Observations come in the form (x, y, pitch, yaw)
        This converts our predicted state into a predicted observation, to compare against our true observation
        """
        return x[:4]
    
    def jacobF(self, x, u):
        """
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
        pitch = x[2, 0]
        yaw = x[3, 0]
        v = u[0, 0]
        jF = np.array([
            [1.0, 0.0, -self.dt * v * math.cos(yaw) * math.sin(pitch), -self.dt * v * math.sin(yaw) * math.cos(pitch), self.dt * math.cos(yaw) * math.cos(pitch)],
            [0.0, 1.0, -self.dt * v * math.sin(yaw) * math.sin(pitch), self.dt * v * math.cos(yaw) * math.cos(pitch), self.dt * math.sin(yaw) * math.cos(pitch)],
            [0.0, 0.0, 1.0, 0.0, 0.0],
            [0.0, 0.0, 0.0, 1.0, 0.0],
            [0.0, 0.0, 0.0, 0.0, 1.0]])

        return jF

    def jacobH(self):
        # Jacobian of Observation Model
        """
        x,y,pitch,yaw
        [1,0,0,0,0]
        [0,1,0,0,0]
        [0,0,1,0,0]
        [0,0,0,1,0]
        ])
        """
        return np.array([
            [1, 0, 0, 0, 0],
            [0, 1, 0, 0, 0],
            [0, 0, 1, 0, 0],
            [0, 0, 0, 1, 0]
        ])

    def predict(self, x_est, p_est, u, dt):
        """
        Predict new state and covariance based on past state, covariance and control inputs
        :param x_est: previous state estimate
        :param p_est: previous state covariance estimate
        :param u: control inputs (d_pitch/d_t, d_yaw/d_t, v)
        :param dt: time diff since last input
        :return: new predicted state and covariance
        """
        self.dt = dt
        # predicted state
        x_pred = self.simple_motion_model(x_est, u)

        x_pred[2:3] %= (2 * math.pi)  # correct angles to principle values

        J_F = self.jacobF(x_est, u)

        p_pred = np.matmul(np.matmul(J_F, p_est), J_F.T) + self.Q # Covariance of our prediction

        return x_pred, p_pred

    def correct(self, x_pred, p_pred, z_obs, R):
        """
        Correct current model based on observation and its covariance
        :param x_pred: predicted x
        :param p_pred: prediction covariance
        :param z_obs: observation
        :param R: covariance of observation
        :return:
        """
        z_pred = self.observation_model(x_pred)

        z_obs[0, 2:3] %= 2 * math.pi  # principle value of pitch and yaw

        J_H = self.jacobH()
        S = np.matmul(np.matmul(J_H, p_pred), J_H.T) + R # covariance of z_pred

        # Difference between expected observation and true observation
        y = z_obs - z_pred.T
        y[0, 2:3] %= 2

        # Kalman gain, scales by covariance of the observation measurement. High covariance -> K has a small determinant
        K = np.matmul(np.matmul(p_pred, J_H.T), np.linalg.inv(S))

        # scale the expected - predicted obs by the Kalman gain - small Kalman gain means we don't trust the obs
        x_corrected = x_pred + np.matmul(K, y.T)
        x_corrected[2:3, 0] %= 2 * math.pi

        # formula for the covariance update, but factorised for simplicity.
        p_corrected = np.matmul((np.eye(len(x_corrected)) - np.matmul(K, J_H)), p_pred)

        return x_corrected, p_corrected


if __name__ == '__main__':
    error = []
    #for i in range(0, 100, 5):
    i = 100
    P_AR_OBSERVED = i / 100
    EKF = ekf(10)
    est_data, true_data, gps_data = EKF.runFilter()
    plt.figure()
    plt.plot(gps_data[0], gps_data[1],'b', lw=0.5)
    plt.plot(true_data[0], true_data[1],'r', lw=0.5)
    plt.title("True position (orange) and GPS measurement (blue)")
    plt.figure()
    plt.plot(est_data[0], est_data[1],'b', lw=0.5)
    plt.plot(true_data[0], true_data[1],'r', lw=0.5)
    plt.title("True position (orange) and Kalman filter position estimate (blue)")
    
    plt.show()

    print("Average error per time-step with position and AR tag distances: ")



        # compute some form of the RMSE for the kalman estimate and for the GPS only estimate. 
    avg_error_kalman = (np.average(((np.array(est_data[0]) - np.squeeze(np.array(true_data[0])).astype(np.float))**2)) + np.average(((np.array(est_data[1]) - np.squeeze(np.array(true_data[1])).astype(np.float))**2)))/2

    avg_error_gps = (np.average(((np.array(gps_data[0]).astype(np.float) - np.array(true_data[0]).astype(np.float))**2)) + np.average(((np.array(gps_data[1]).astype(np.float) - np.array(true_data[1]).astype(np.float))**2)))/2
    error.append(avg_error_kalman)
    print("Kalman, + AR: ", avg_error_kalman)
    print("Pose only: ", avg_error_gps)
