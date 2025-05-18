#!/usr/bin/env python3
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
    HEATER_IR_SENSOR_READ_SEND_FRAME = 0x4B1
    DIRT_IR_SENSOR_READ_FRAME = 0x4A1

    # ROS2 SERVICES
    HEATER_SERVICE = "/science/heater"
    IR_TOPIC = "/science/IRSensors"

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
            frame_id=self.HEATER_IR_SENSOR_READ_SEND_FRAME,
            run_can=True,
        )
        self.dirt_sensor = IntegerSensor(
            bus=self.bus,
            logger=logger,
            frame_id=self.DIRT_IR_SENSOR_READ_FRAME,
            run_can=True,
        )
        self.add_sensor("heater_IR_sensor", self.heater_sensor)
        self.add_sensor("dirt_IR_sensor", self.dirt_sensor)

        # Create Services and Publishers
        self.create_service(KilnCommand, self.HEATER_SERVICE, self.toggle_callback)

        self.IR_publisher = self.create_publisher(KilnData, self.IR_TOPIC, 10)
        self.create_timer(0.5, self.publish_data)

        self.start_can()

    def check_temp(self, temp: int):
        """ Turn off the kiln if the temperature has been reached """
        if not self.heater_control.is_on():
            return

        if self.target_temp < temp:
            self.heater_control.start()
        else:
            self.heater_control.stop()

    def convert_to_temp(self, ir_reading: int):
        """ Convert the IR sensor readings from ADC to degrees celsius """
        return int(ir_reading / 175)

    def publish_data(self):
        """ Publish the current readings from the sensors """
        heater_data = self.convert_to_temp(self.heater_sensor.get_sensor_value())
        dirt_data = self.convert_to_temp(self.dirt_sensor.get_sensor_value())

        msg = KilnData()
        msg.state = self.heater_control.is_on()
        msg.temp = [heater_data, dirt_data]

        self.check_temp(heater_data)

    def toggle_callback(self, request: KilnCommand_Request, response: KilnCommand_Response) -> KilnCommand_Response:
        """ Turn the kiln on/off and set the target temperature """
        if request.state and not self.heater_control.is_on():
            self.get_logger().info("Turning Heater ON")
            self.heater_control.start()
        elif not request.state and self.heater_control.is_on():
            self.get_logger().info("Turning Heater OFF")
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
