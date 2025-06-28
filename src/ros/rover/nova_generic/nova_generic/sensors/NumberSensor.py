#!/usr/bin/env python3

import jcan
from nova_generic.sensors import Sensor
from nova_generic.sensors.number_sensor_parameters import number_sensor_parameters
from nova_interfaces.msg import NumberReading

class NumberSensor(Sensor):
    """Class to represent a number sensor"""

    SCALE_PARAM = "scale"
    OFFSET_PARAM = "offset"

    def __init__(self):
        super().__init__("NumberSensor")

        # declare parameters
        self.param_listener = number_sensor_parameters.ParamListener(self)
        self.params = self.sensor_param_listener.get_params()

        # define the publisher
        self.publisher = self.create_publisher(NumberReading, self.sensor_params.topic, 10)

    def publish(self):
        pass

    def callback_function(self, frame: jcan.Frame):
        pass
