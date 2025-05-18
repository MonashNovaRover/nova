#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Carousel stepper control
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: CacheNode
TOPICS: None
SERVICES:
    - server: /science/carousel_service
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    science
AUTHOR(S):	Felicity Matthews
CREATION:	18/05/2025
EDITED:		18/05/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import time

import jcan
import rclpy
from jcan import Frame
from nova_interfaces.srv import KilnCommand, KilnCommand_Response, KilnCommand_Request
from python_control.ControllerNode import ControllerNode
from rclpy.executors import MultiThreadedExecutor

class URCCarousel(ControllerNode):

    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # ROS ACTION
    # The name of the ROS action to use
    CAROUSEL_SERVICE = "/science/carousel_service"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    STEPPER_STM_SEND = 0x0A0

    # SENDING COMMAND IDS
    # Add any CONTROL command ids here
    STEPPER_SET_DIRECTION_COMMAND_ID = 0x00
    STEPPER_ENABLE_COMMAND_ID = 0x01
    STEPPER_DISABLE_COMMAND_ID = 0x02
    STEPPER_MOVE_COMMAND_ID = 0x03

    # DIRECTIONS
    DIRECTION_CLOCKWISE = 0x01
    DIRECTION_ANTICLOCKWISE = 0x00

    # LIMITS
    MAX_STEPS = 240   # this equals 6 cuvette rotations

    def __init__(self):
        super().__init__(name="URCCarousel", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        # Current State
        self.is_active = False
        self.is_clockwise = True

        # Add Services
        self.stepper_service = self.create_service(KilnCommand, self.CAROUSEL_SERVICE, self.stepper_service_callback)

        ## Start the CAN bus
        self.start_can()

    def update_status(self, status: bool):
        """ Updates the current status of the stepper and returns whether any commands were sent """
        if status == self.is_active:
            return False

        if status:
            self.get_logger().info("Turning stepper on")
            self.bus.send(Frame(self.STEPPER_STM_SEND, [self.STEPPER_ENABLE_COMMAND_ID]))
        else:
            self.get_logger().info("Turning stepper off")
            self.bus.send(Frame(self.STEPPER_STM_SEND, [self.STEPPER_DISABLE_COMMAND_ID]))

        return True

    def update_direction(self, clockwise: bool):
        """ Updates the direction of the stepper and returns whether any commands were sent """
        if clockwise == self.is_clockwise:
            return False

        if clockwise:
            self.bus.send(Frame(self.STEPPER_STM_SEND, [self.STEPPER_SET_DIRECTION_COMMAND_ID, self.DIRECTION_CLOCKWISE]))
        else:
            self.bus.send(Frame(self.STEPPER_STM_SEND, [self.STEPPER_SET_DIRECTION_COMMAND_ID, self.DIRECTION_ANTICLOCKWISE]))

        return True

    def stepper_service_callback(self, request: KilnCommand_Request, response: KilnCommand_Response):
        """
        Upon a request to move the stepper, the stepper is activated and moved however many steps in the specified direction
        """
        self.get_logger().info(f'received request: {request}')

        try:
            status_updated = self.update_status(request.state)

            if request.target == 0 or not self.is_active:
                response.success = True
                return response

            if status_updated:
                time.sleep(0.1)

            # if the move stepper number is negative it is clockwise
            direction_updated = self.update_direction(request.target < 0)
            if direction_updated:
                time.sleep(0.1)

            steps_left = abs(request.target)
            while steps_left > 0:
                moving_steps = min(steps_left, self.MAX_STEPS)
                self.get_logger().info(f'Carousel moving {moving_steps} steps {"CLOCKWISE" if self.is_clockwise else "ANTICLOCKWISE"}')

                self.bus.send(jcan.Frame(self.STEPPER_STM_SEND, [self.STEPPER_MOVE_COMMAND_ID, moving_steps]))

                steps_left -= moving_steps
                if steps_left > 0:
                    time.sleep(0.1)

            response.success = True

        except Exception as e:
            self.get_logger().error(f"Failed to send Stepper command over CAN: {e}")
            response.success = False

        return response

def main():
    rclpy.init()
    node = URCCarousel()
    rclpy.spin(node, executor=MultiThreadedExecutor())
    rclpy.shutdown()


if __name__ == "__main__":
    main()