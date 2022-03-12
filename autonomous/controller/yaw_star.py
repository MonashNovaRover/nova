
from math_utils.controller_math import *
from config.runtime_params import *


class Turning:
    def __init__(self, yaw_star=False):
        self.yaw_star = yaw_star

        # Normal turning params
        self.previously_turned = False

        # yaw_star params
        self.MAX_YAW = np.pi / 7.5
        self.MAX_TRAVERSAL_DISTANCE = 1
        self.target_yaw = 0
        # Variables
        self.star_state = 0
        self.first_drive = True
        self.direction = -1

    def run(self, yaw_diff, state, position_vector):
        self.state = state

        if self.yaw_star:
            drive = self.run_star(yaw_diff, position_vector)
        else:
            drive = self.run_normal(yaw_diff, position_vector)
        return drive

    def run_normal(self, yaw_diff, position_vector):
        if abs(yaw_diff) >= min_yaw_difference:
            # turn at a rate determined by the tank_turn_target_yaw_rate function
            steer_fraction = tank_turn_target_yaw_rate(yaw_diff)
            drive_fraction = 0.0
            self.previously_turned = True

        elif self.previously_turned:
            # need to send a zero wheel command after turning before we drive
            steer_fraction = 0.0
            drive_fraction = 0.0
            self.previously_turned = False

        else:
            # drive in straight line toward waypoint at determined velocity
            drive_fraction = crow_fly_target_velocity((self.state.x, self.state.y), self.target_waypoint)
            steer_fraction = 0.0

        return {'steer': steer_fraction, 'drive': drive_fraction}

    def run_star(self, yaw_diff, position_vector):
        steer_fraction = 0.0
        drive_fraction = 0.0
        if abs(yaw_diff) > self.MAX_YAW:
            # Big turn, either drive straight or turn
            if self.star_state == 0:
                # From straight line to first yaw
                self.target_yaw = np.sign(yaw_diff) * (abs(yaw_diff) - self.MAX_YAW)
                steer_fraction = tank_turn_target_yaw_rate(yaw_diff)
                drive_fraction = 0.05
                # Update state
                self.star_state = 1

            elif self.star_state == 1:
                # Check if keep yawing
                if yaw_diff > self.target_yaw:
                    # Keep turning
                    steer_fraction = tank_turn_target_yaw_rate(yaw_diff)
                    drive_fraction = 0.05
            else:
                # Swap to drive mode
                dist = 0.5 * self.MAX_TRAVERSAL_DISTANCE if self.first_drive else self.MAX_TRAVERSAL_DISTANCE
                self.star_state = 2
                self.target_yaw = 0
                self.target_pose = [position_vector[0] + dist * self.direction * np.cos(current_orientation),
                                    position_vector[1] + dist * self.direction * np.sin(current_orientation), 0]
                steer_fraction = 0.0
                drive_fraction = 0.0
        elif self.star_state == 2:
            # Check if keep driving
            dist = distance(position_vector, self.target_pose)
            if abs(dist) > 0.01:
                # Keep driving
                drive_fraction = 0.1 * self.direction
                steer_fraction = 0.0
            else:
                # Return to turning
                steer_fraction = 0.0
                drive_fraction = 0.0

                # Update variables
                self.direction = self.direction * -1
                self.star_state = 0
                self.first_drive = False

        elif self.MAX_YAW > abs(yaw_diff) > min_yaw_difference:
            # Turn on the spot
            steer_fraction = tank_turn_target_yaw_rate(yaw_diff)
            drive_fraction = 0.05
        else:
            # Reset constants
            self.star_state = 0
            self.first_drive = True
            self.direction = -1

            # drive in straight line toward waypoint at determined velocity
            drive_fraction = crow_fly_target_velocity((self.state.x, self.state.y), self.target_waypoint)

        return {'steer': steer_fraction, 'drive': drive_fraction}
