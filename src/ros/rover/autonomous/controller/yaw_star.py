
from math_utils.controller_math import *
from config.runtime_params import *


class Turning:
    def __init__(self, logger):

        self.yaw_star = yaw_star_conf

        self.logger = logger
        # Normal turning params
        self.previously_turned = False

        # yaw_star params
        self.MAX_YAW = np.pi / 7.5
        self.MAX_TRAVERSAL_DISTANCE = 0.3
        self.target_yaw = 0
        # Variables
        self.star_state = 0
        self.first_drive = True
        self.direction = -1

    def run(self, yaw_diff, state, position_vector, target_waypoint, current_orientation):
        self.state = state
        self.target_waypoint = target_waypoint
        if self.yaw_star:
            drive = self.run_star(yaw_diff, position_vector, current_orientation)
        else:
            drive = self.run_normal(yaw_diff, position_vector)
        return drive

    def run_normal(self, yaw_diff, position_vector):
        if abs(yaw_diff) >= min_yaw_difference:
            self.logger.info('Normal: turning, yaw_diff = ' + str(yaw_diff))
            # turn at a rate determined by the tank_turn_target_yaw_rate function
            steer_fraction = tank_turn_target_yaw_rate(yaw_diff)
            drive_fraction = 0.05
            self.previously_turned = True

        elif self.previously_turned:
            self.logger.info('Normal: swap')
            # need to send a zero wheel command after turning before we drive
            steer_fraction = 0.0
            drive_fraction = 0.0
            self.previously_turned = False

        else:
            self.logger.info('Normal: driving')
            # drive in straight line toward waypoint at determined velocity
            drive_fraction = crow_fly_target_velocity((self.state.x, self.state.y), self.target_waypoint)
            steer_fraction = 0.0

        return {'steer': steer_fraction, 'drive': drive_fraction}

    def run_star(self, yaw_diff, position_vector, current_orientation):
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
                drive_fraction = 0.05
                # Update state
                self.star_state = 1

            elif self.star_state == 1:
                # Check if keep yawing
                self.logger.info('Star: 1: ' + str(yaw_diff) + ' ' + str(self.target_yaw))
                if abs(yaw_diff) > abs(self.target_yaw):
                    # Keep turning
                    self.logger.info('Star: keep_yaw')
                    steer_fraction = tank_turn_target_yaw_rate(yaw_diff)
                    drive_fraction = 0.05
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
                    drive_fraction = 0.1 * self.direction
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
            drive_fraction = 0.05
        else:
            self.logger.info('Star: driving')
            # Reset constants
            self.star_state = 0
            self.first_drive = True
            self.direction = -1

            # drive in straight line toward waypoint at determined velocity
            drive_fraction = crow_fly_target_velocity((self.state.x, self.state.y), self.target_waypoint)

        return {'steer': steer_fraction, 'drive': drive_fraction}
