#!/usr/bin/env python3

from python_control.sensors.IntegerSensor import IntegerSensor
import rclpy
from python_control.ControllerNode import ControllerNode
from science_interfaces.msg import BMESensor

class URCBMESensor(ControllerNode):

    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # RECEIVING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    BME_TEMP_RECV_FRAME_ID = 0x4F3
    BME_HUMIDITY_RECV_FRAME_ID = 0x4F2
    BME_PRESSURE_RECV_FRAME_ID = 0x4F4
    BME_ALTITUDE_RECV_FRAME_ID = 0x4F5

    # CONTROL NAMES
    # Add any CONTROL names here
    BME_TEMP_NAME = "bme_temperature"
    BME_HUMIDITY_NAME = "bme_humidity"
    BME_PRESSURE_NAME = "bme_pressure"
    BME_ALTITUDE_NAME = "bme_altitude"

    # SENSOR CONSTANTS
    BME_TEMP_FACTOR = 100
    BME_HUMIDITY_FACTOR = 100


    def __init__(self):
        super(URCBMESensor, self).__init__(name="URCBMESensor", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        # ## Add Publishers
        self.bme_publisher = self.create_publisher(BMESensor, "/science/bme_sensor", 10)

        # ## Create Sensors
        self.temperature = IntegerSensor(
            bus=self.bus,
            logger=logger,
            frame_id=self.BME_TEMP_RECV_FRAME_ID,
            run_can=True,        
        )
        self.humidity = IntegerSensor(
            bus=self.bus,
            logger=logger,
            frame_id=self.BME_HUMIDITY_RECV_FRAME_ID,
            run_can=True,        
        )
        self.pressure = IntegerSensor(
            bus=self.bus,
            logger=logger,
            frame_id=self.BME_PRESSURE_RECV_FRAME_ID,
            run_can=True,
        )
        self.altitude = IntegerSensor(
            bus=self.bus,
            logger=logger,
            frame_id=self.BME_ALTITUDE_RECV_FRAME_ID,
            run_can=True,
        )

        ## Add the controllers to the node's of controllers
        self.add_sensor(sensor_name=self.BME_TEMP_NAME, sensor=self.temperature)
        self.add_sensor(sensor_name=self.BME_HUMIDITY_NAME, sensor=self.humidity)
        self.add_sensor(sensor_name=self.BME_PRESSURE_NAME, sensor=self.pressure)
        self.add_sensor(sensor_name=self.BME_ALTITUDE_NAME, sensor=self.altitude)

        self.create_timer(1.0, self.publish_data)

        ## Start the CAN bus
        self.start_can()

    def publish_data(self):
        msg = BMESensor()
        msg.temperature = float(self.temperature.get_sensor_value() / self.BME_TEMP_FACTOR)
        msg.humidity = float(self.humidity.get_sensor_value() / self.BME_HUMIDITY_FACTOR)
        msg.pressure = int(self.pressure.get_sensor_value())
        msg.altitude = int(self.altitude.get_sensor_value())
        self.bme_publisher.publish(msg)

def main():
    rclpy.init()
    node = URCBMESensor()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()