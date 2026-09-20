#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
<insert purpose here>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: Auto
TOPICS:
  - publisher: <topic> [<msg type>]
SERVICES:
	- service: <service> [<srv type>]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        <package>
AUTHOR(S):      <insert your name>
CREATION:       <current date>
EDITED:         <current date>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node

from nova_interfaces.msg import GPSData
from nova_interfaces.srv import CartographerCommand

class Auto(Node):

    SPEED_PARAM = "speed"

    def __init__(self):
        super().__init__("Auto")
        
        # define variables
        #

        self.declare_parameter(self.SPEED_PARAM, 0.0000005)

        # how often the update loop is called, can be anything you want (in sec)
        self.PERIOD = 0.5

        # creates a ROS2 timer that call the update function every interval
        self.update_timer = self.create_timer(self.PERIOD, lambda: self.update(self.PERIOD))

        self.current_pos = (38.406787449642906, -110.79183322459582)
        self.target_pos = self.current_pos

        # add publishers
        #
        self.gps_publisher = self.create_publisher(GPSData, '/gps_rover/fix_custom', 10)
        
        # add services
        #

        self.cartographer_service = self.create_service(CartographerCommand, '/autonomous/cartographer_command', self.cartographer_callback)
        
        self.get_logger().info(f"{self.get_name()} started.")

    def cartographer_callback(self, request, response):
        # make sure there is a goal position
        if len(request.goals) <= 0:
            response.success = False
            return response

        goal_pos = request.goals[-1] # goal pos is the last provided position

        # update target position
        self.target_pos = (goal_pos.latitude, goal_pos.longitude)
        self.get_logger().info(f"Target position updated to: {self.target_pos}")

        response.success = True
        return response

    def update(self, delta_time):
        displacement = (
		    self.target_pos[0] - self.current_pos[0], 
		    self.target_pos[1] - self.current_pos[1]
	    )
        distance = (displacement[0] ** 2 + displacement[1] ** 2) ** 0.5
	
        if distance > 0:
            # normalise displacement vector
            direction = (
                displacement[0] / distance, 
                displacement[1] / distance
            )
            
            # get the velocity to move
            velocity = (
                direction[0] * self.get_parameter(self.SPEED_PARAM).value, 
                direction[1] * self.get_parameter(self.SPEED_PARAM).value
            )
            
            # calculate movement for this update
            delta_pos = (
                velocity[0] * delta_time,
                velocity[1] * delta_time
            )
            
            # make sure we don't overshoot
            delta_pos_dist = (delta_pos[0] ** 2 + delta_pos[1] ** 2) ** 0.5
            if delta_pos_dist > distance:
                delta_pos = displacement
            
            # update current position
            self.current_pos = (
                self.current_pos[0] + delta_pos[0],
                self.current_pos[1] + delta_pos[1]
            )

        msg = GPSData()

        msg.latitude, msg.longitude = self.current_pos[0], self.current_pos[1]

        self.gps_publisher.publish(msg)

def main():
    rclpy.init()
    node = Auto()
    rclpy.spin(node)
    rclpy.shutdown()



if __name__ == '__main__':
    main()
