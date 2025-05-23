#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: URC Heater Control
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: URCHeater
TOPICS:
    - publisher: /science/kiln_data
SERVICES:
    - server: "/science/heater"
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    science
AUTHOR(S):	Felicity Matthews
CREATION:	20/05/2025
EDITED:		24/05/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from nova_interfaces.msg import KilnData
from nova_interfaces.srv import KilnCommand, KilnCommand_Request, KilnCommand_Response
from python_control.ControllerNode import ControllerNode
from python_control.controllers.ToggleController import ToggleController
from python_control.controls.ToggleControl import ToggleControl
from python_control.sensors.IntegerSensor import IntegerSensor


class URCHeater(ControllerNode):
    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    HEATER_CONTROL_SEND_FRAME = 0x0B0
    HEATER_NTC_SENSOR_READ_SEND_FRAME = 0x4B1
    DIRT_NTC_SENSOR_READ_FRAME = 0x4A1

    # ROS2 SERVICES
    HEATER_SERVICE = "/science/heater"
    NTC_TOPIC = "/science/kiln_data"

    # CONTROL NAMES
    # Add any CONTROL names here
    HEATER_CONTROL = "heater"

    # SENDING COMMAND IDS
    # Add any CONTROL command ids here
    HEATER_CONTROL_ID = 0x05
    HEATER_SEND_ON = 0xFF
    HEATER_SEND_OFF = 0x00

    def __init__(self):
        super().__init__(name="urc_heater", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        self.is_active = False
        self.target_temp = 50

        # Add Controls and Controllers
        self.heater_control = ToggleControl(logger=logger, on=False)
        self.heater_controller = ToggleController(
            logger=logger,
            bus=self.bus,
            frame_id=self.HEATER_CONTROL_SEND_FRAME,
            control_id=self.HEATER_CONTROL_ID,
            toggle_command_on=self.HEATER_SEND_ON,
            toggle_command_off=self.HEATER_SEND_OFF,
            control=self.heater_control,
        )
        self.add_controller(self.HEATER_CONTROL, self.heater_controller)

        # Add Sensors
        self.heater_sensor = IntegerSensor(
            bus=self.bus,
            logger=logger,
            frame_id=self.HEATER_NTC_SENSOR_READ_SEND_FRAME,
            run_can=True,
        )
        self.dirt_sensor = IntegerSensor(
            bus=self.bus,
            logger=logger,
            frame_id=self.DIRT_NTC_SENSOR_READ_FRAME,
            run_can=True,
        )
        self.add_sensor("heater_NTC_sensor", self.heater_sensor)
        self.add_sensor("dirt_NTC_sensor", self.dirt_sensor)

        # Create Services and Publishers
        self.create_service(KilnCommand, self.HEATER_SERVICE, self.toggle_callback)

        self.NTC_publisher = self.create_publisher(KilnData, self.NTC_TOPIC, 10)
        self.publish_timer = self.create_timer(0.5, self.publish_data)

        self.start_can()

    def check_temp(self, temp: int):
        """ Turn off the kiln if the temperature has been reached """
        if self.target_temp > temp:
            self.heater_control.start()
        else:
            self.heater_control.stop()

    def publish_data(self):
        """ Publish the current readings from the sensors """
        heater_data = self.heater_sensor.get_sensor_value()
        dirt_data = self.dirt_sensor.get_sensor_value()

        if self.is_active:
            self.check_temp(heater_data)

        msg = KilnData()
        msg.state = self.is_active
        msg.temp = [heater_data, dirt_data]
        self.NTC_publisher.publish(msg)

    def toggle_callback(self, request: KilnCommand_Request, response: KilnCommand_Response) -> KilnCommand_Response:
        """ Turn the kiln on/off and set the target temperature """
        if request.state and not self.is_active:
            self.get_logger().info("Turning Heater ON")
            self.is_active = True
        elif not request.state and self.is_active:
            self.get_logger().info("Turning Heater OFF")
            self.is_active = False
            self.heater_control.stop()

        self.target_temp = request.target
        self.get_logger().info(f'Target temp: {self.target_temp}')

        response.success = True
        return response


def main():
    rclpy.init()
    urc_heater = URCHeater()
    rclpy.spin(urc_heater)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
