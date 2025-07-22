#!/usr/bin/env python3

from python_control.controls.Direction import Direction
from python_control.controls.OneAxisVelocityControl import OneAxisVelocityControl
from python_control.controllers.CMDVelocityController import CMDVelocityController
import rclpy
from python_control.ControllerNode import ControllerNode
from std_srvs.srv import SetBool


class URCMixers(ControllerNode):

    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    MIXER_1_SEND_FRAME_ID = 0x0D1
    # MIXER_2_SEND_FRAME_ID = 0x0D2

    # ROS2 SERVICES
    MIXER_SERVICE = "/science/mixers"

    # CONTROL NAMES
    # Add any CONTROL names here
    MIXER_1_NAME = "mixer_1"
    # MIXER_2_NAME = "mixer_2"

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    MIXER_1_MAX_PERCENT = 0.75
    # MIXER_2_MAX_PERCENT = 0.75
    
    # CONTROL DIRECTIONS
    # Add any CONTROL DIRECTIONS here
    MIXER_1_CLOCKWISE = Direction.POSITIVE
    MIXER_1_DOWN = Direction.NEGATIVE
    # MIXER_2_CLOCKWISE = Direction.POSITIVE
    # MIXER_2_COUNTERCLOCKWISE = Direction.NEGATIVE    

    def __init__(self):
        super(URCMixers, self).__init__(name="URCMixers", can_bus=self.CAN_BUS)
        logger = self.get_logger()


        # Create publishers
        self.create_service(SetBool, self.MIXER_SERVICE, self.mixer_callback)
 

        ## Create controls
        self.mixer_1 = OneAxisVelocityControl(
            logger=logger,
            max_percent=self.MIXER_1_MAX_PERCENT,
            direction=self.MIXER_1_CLOCKWISE,
        )
        # self.mixer_2 = OneAxisVelocityControl(
        #     logger=logger,
        #     max_percent=self.MIXER_2_MAX_PERCENT,
        #     direction=self.MIXER_2_CLOCKWISE,
        # )


        ## Create controllers
        self.mixer_1_controller = CMDVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.MIXER_1_SEND_FRAME_ID,
            control=self.mixer_1
        )
        # self.mixer_2_controller = CMDVelocityController(
        #     logger=logger,
        #     bus=self.bus,
        #     frame_id=self.MIXER_2_SEND_FRAME_ID,
        #     control=self.mixer_2
        # )

        ## Add the controllers to the node's of controllers
        self.add_controller(self.MIXER_1_NAME, self.mixer_1_controller)
        # self.add_controller(self.MIXER_2_NAME, self.mixer_2_controller)

        ## Start the CAN bus
        self.start_can()

    def mixer_callback(self, request, response):
        try:
            if request.data:
                self.mixer_1.update_velocity(1.0)
                # self.mixer_2.update_velocity(1.0)
                response.message = "Mixers Started"  
            else:
                self.mixer_1.update_velocity(0.0)
                # self.mixer_2.update_velocity(0.0)
                response.message = "Mixers Stopped"    
            response.success = True
        except Exception as e:
            self.get_logger().error("Error in mixer_callback: {0}".format(e))
            response.success = False
            response.message = str(e)
        return response
        
            
def main():
    rclpy.init()
    node = URCMixers()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()