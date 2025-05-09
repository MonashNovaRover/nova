#!/usr/bin/env python3

import time
from python_control.controls.ToggleControl import ToggleControl
from python_control.controllers.ToggleController import ToggleController
from python_control.sensors.ToggleCommandSensor import ToggleCommandSensor
from rclpy.executors import MultiThreadedExecutor
import rclpy
from python_control.ControllerNode import ControllerNode
from std_srvs.srv import SetBool


class URCUVVisLeds(ControllerNode):

    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    LED_1_SEND_FRAME_ID = 0x0A0
    LED_2_SEND_FRAME_ID = 0x0A0

    # RECIEVING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    # LED_RECV_FRAME_ID = 0x451

    # RECV COMMAND IDS
    # Add any SENSOR command ids here
    # LED_1_COMMAND = 0x01
    # LED_2_COMMAND = 0x02
    # LED_ON_ID = 0x01
    # LED_OFF_ID = 0x00

    # ROS2 SERVICES
    LED_1_SERVICE = "/science/uv_vis_led_1"
    LED_2_SERVICE = "/science/uv_vis_led_2"

    # CONTROL NAMES
    # Add any CONTROL names here
    LED_1_NAME = "led_1"
    LED_2_NAME = "led_2"

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    LED_1_MAX_PERCENT = 0.75
    LED_2_MAX_PERCENT = 0.75
    
    # CONTROL COMMANDS
    # Add any CONTROL command ids here
    LED_1_COMMAND = 0x04
    LED_2_COMMAND = 0x05
    LED_ON_COMMAND = 0xFF
    LED_OFF_COMMAND = 0x00

    TIMEOUT = 2.0

    def __init__(self):
        super(URCUVVisLeds, self).__init__(name="URCUVVisLeds", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        # self.bus.set_id_filter([self.LED_RECV_FRAME_ID])


        # Create publishers
        self.create_service(SetBool, self.LED_1_SERVICE, self.led_1_callback)
        self.create_service(SetBool, self.LED_2_SERVICE, self.led_2_callback)
 

        # Create Sensors
        # self.led_1_sensor = ToggleCommandSensor(
        #     logger=logger,
        #     bus=self.bus,
        #     frame_id=self.LED_RECV_FRAME_ID,
        #     state_id_on=self.LED_ON_ID,
        #     state_id_off=self.LED_OFF_ID,
        #     control_id=self.LED_1_COMMAND,
        # )
        #
        # self.led_2_sensor = ToggleCommandSensor(
        #     logger=logger,
        #     bus=self.bus,
        #     frame_id=self.LED_RECV_FRAME_ID,
        #     state_id_on=self.LED_ON_ID,
        #     state_id_off=self.LED_OFF_ID,
        #     control_id=self.LED_2_COMMAND,
        # )

        # Add sensors to the node
        # self.add_sensor(self.LED_1_NAME, self.led_1_sensor)
        # self.add_sensor(self.LED_2_NAME, self.led_2_sensor)

        ## Create controls
        self.led_1 = ToggleControl(
            logger=logger,
        )
        self.led_2 = ToggleControl(
            logger=logger,
        )


        ## Create controllers
        self.led_1_controller = ToggleController(
            logger=logger,
            bus=self.bus,
            frame_id=self.LED_1_SEND_FRAME_ID,
            control=self.led_1,
            toggle_command_on=self.LED_ON_COMMAND,
            toggle_command_off=self.LED_OFF_COMMAND,
            control_id=self.LED_1_COMMAND,
        )
        self.led_2_controller = ToggleController(
            logger=logger,
            bus=self.bus,
            frame_id=self.LED_2_SEND_FRAME_ID,
            control=self.led_2,
            toggle_command_on=self.LED_ON_COMMAND,
            toggle_command_off=self.LED_OFF_COMMAND,
            control_id=self.LED_2_COMMAND,
        )

        ## Start the CAN busget_name
        self.start_can()

    def led_callback(self, request, response, controller: ToggleController, control_name: str):
        self.get_logger().info("LED Callback: {0}".format(request.data))
        control = controller.get_control()
        try:
            if request.data:
                control.start()
                response.message = "{0} Started".format(control_name)
            else:
                control.stop()
                response.message = "{0} Stopped".format(control_name)

            controller.control_send_callback()
            response.success = True

        except Exception as e:
            self.get_logger().error("Error in led_callback: {0}".format(e))
            response.success = False
            response.message = str(e)
        self.get_logger().info("LED Callback Response: {0}".format(response))
        return response
    
    def led_1_callback(self, request, response):
        return self.led_callback(request, response, self.led_1_controller, "LED 1")

    def led_2_callback(self, request, response):
        return self.led_callback(request, response, self.led_2_controller,"LED 2")
        
            
def main():
    rclpy.init()
    node = URCUVVisLeds()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()