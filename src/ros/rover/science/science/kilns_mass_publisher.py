#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 receiver code for the kilns and bilns data publisher.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ PACKAGE:     electronics
AUTHOR(S):   Niko Verrios
CREATION:    18/03/2023
EDITED:      18/03/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
import jcan

# import custom messages
from core.msg import KilnData


def convert_to_grams(data):
    return int.from_bytes(data, 'little', signed=True)


class KilnDataPublisher(Node):

    def __init__(self):
        super().__init__("kiln data publisher")
        #publisher to publish the data from the kilns.
        self.publisher = self.create_publisher(KilnData, "/science/kiln_data", 10)

        #declare parameters
        self.declare_parameter("canbus", "can1")

        #initialise the can bus
        self.bus = jcan.Bus()

        # TODO: Update these filter masks.
        self.bus.set_id_filter([0x4A1, 0x4B1])

        self.bus.add_callback(0x4A1, self.get_callback)
        self.bus.add_callback(0x4B1, self.get_callback)

        #create timers
        self.can_spin_timer = self.create_timer(0.01, self.bus.spin)
        self.publish_status_timer = self.create_timer(1/50, self.publish_status)

        #open the can bus
        self.bus.open(self.get_parameter("canbus").value)


    def get_callback(self):
        """
        Returns a callback function for the kilns
        :return:
        """
        def callback(frame):

            if frame.id == 0x4A1:
                self.mass_g = convert_to_grams(frame.data[:2])  
                self.kiln_id = 0

            elif frame.id == 0x4B1:
                self.mass_g = convert_to_grams(frame.data[:2])
                self.kiln_id = int(frame.data[1])

                
        return callback


def main():
    rclpy.init()
    publisher_node = KilnDataPublisher()
    rclpy.spin(publisher_node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()