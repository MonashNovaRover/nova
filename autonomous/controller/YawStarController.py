__package__ = "autonomous"

from math_utils.controller_math import *
from config.runtime_params import *
from controller.ControllerInterface import ControllerInterface
import numpy as np

class YawStarController(ControllerInterface):
    def __init__(self, logger):
        self.target_waypoint = None
        self.state = None
        self.logger = logger

        # yaw_star params
        self.MAX_YAW = np.pi / 7.5
        self.MAX_TRAVERSAL_DISTANCE = 0.3
        self.target_yaw = 0
        # Variables
        self.star_state = 0
        self.first_drive = True
        self.direction = -1

    def get_drive_command(self, target_waypoint, state, goal = None, gate = None):
        self.state = state
        self.target_waypoint = target_waypoint
        position_vector = np.array([self.state.x, self.state.y, 0])
        target_vector = np.array([self.target_waypoint[0], self.target_waypoint[1], 0])

        desired_orientation = target_vector - position_vector
        current_orientation = np.array([np.cos(self.state.yaw), np.sin(self.state.yaw), 0])

        yaw_diff = yaw_difference(current_orientation, desired_orientation)
        
        drive = self.yaw_star(yaw_diff, position_vector, current_orientation)
        self.log(drive)
        return drive

    def yaw_star(self, yaw_diff, position_vector, current_orientation):
        steer_fraction = 0.0
        drive_fraction = 0.0
        self.logger.warn('Params ' + str(self.star_state) + ' ' + str(self.first_drive)+ ' '+ str(yaw_diff)) 
        if abs(yaw_diff) > self.MAX_YAW:
            # Big turn, either drive straight or turn
            if self.star_state == 0:
                self.logger.info('Star: first_yaw')
                # From straight line to first yaw
                self.target_yaw = np.sign(yaw_diff) * (abs(yaw_diff) - self.MAX_YAW)
                steer_fraction = tank_turn_target_yaw_rate(yaw_diff)
                drive_fraction = turn_drive_fraction
                # Update state
                self.star_state = 1

            elif self.star_state == 1:
                # Check if keep yawing
                self.logger.info('Star: 1: ' + str(yaw_diff) + ' ' + str(self.target_yaw))
                if abs(yaw_diff) > abs(self.target_yaw):
                    # Keep turning
                    self.logger.info('Star: keep_yaw')
                    steer_fraction = tank_turn_target_yaw_rate(yaw_diff)
                    drive_fraction = turn_drive_fraction 
                else:
                    # Swap to drive mode
                    self.logger.info('Star: swap_2_drive')
                    dist = 0.5 * self.MAX_TRAVERSAL_DISTANCE if self.first_drive else self.MAX_TRAVERSAL_DISTANCE
                    self.star_state = 2
                    self.target_yaw = 0
                    self.target_pose = [position_vector[0] + dist * self.direction * current_orientation[0],
                                        position_vector[1] + dist * self.direction * current_orientation[1], 0]

                    self.sign = np.sign(np.dot((self.target_pose - position_vector), current_orientation))
  
            elif self.star_state == 2:
                # Check if keep driving
                self.logger.info('Star: Cheque keep driving')
                dist = distance(position_vector, self.target_pose)
                self.logger.info('dist: ' + str(dist) + ' ' + str(self.direction) +' '+str(np.arctan2(position_vector, self.target_pose)))
                self.logger.warn(str(np.dot((self.target_pose - position_vector), current_orientation)))

                sign_new = np.sign(np.dot((self.target_pose - position_vector), current_orientation))
                self.logger.info('remaining: ' + str(dist))
                if abs(dist) > 0.1 and self.sign == sign_new:
                    self.logger.info('Star: keep_driving')
                    # Keep driving
                    drive_fraction = straight_drive_fraction * self.direction
                    steer_fraction = 0.0
                else:
                    self.logger.info('Star: swap_2_turning')
                    # Return to turning
                    steer_fraction = 0.0
                    drive_fraction = 0.0

                    # Update variables
                    self.direction = self.direction * -1
                    self.star_state = 0
                    self.first_drive = False

        elif self.MAX_YAW > abs(yaw_diff) > min_yaw_difference:
            self.logger.info('Star: turning')
            # Turn on the spot
            steer_fraction = tank_turn_target_yaw_rate(yaw_diff)
            drive_fraction = turn_drive_fraction
        else:
            self.logger.info('Star: driving')
            # Reset constants
            self.star_state = 0
            self.first_drive = True
            self.direction = -1

            # drive in straight line toward waypoint at determined velocity
            drive_fraction = crow_fly_target_velocity((self.state.x, self.state.y), self.target_waypoint)

        return {'steer': steer_fraction, 'drive': drive_fraction}
