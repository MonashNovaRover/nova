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
from rclpy.duration import Duration
import jcan

# import custom messages
from core.msg import KilnMassData, KilnMassPollingStatus


def convert_to_grams(data):
    return int.from_bytes(bytes(data), "big", signed=True)/1000


class KilnMassDataPublisher(Node):

    def __init__(self):
        super().__init__("kiln_data_publisher")


        # Print initialisation information
        self.get_logger().warning("\033[92;1mInitialising the Kiln Mass Publisher class.\033[0m")

        #subscriber to polling status
        self.subscriber = self.create_subscription(KilnMassPollingStatus, "/science/kiln_mass_poll_status", self.check_poll_status_callback, 10)
        #publisher to publish the data from the kilns.
        self.publisher = self.create_publisher(KilnMassData, "/science/kiln_mass_data", 1)

        #declare parameters
        self.declare_parameter("canbus", "can1")

        #initialise the can bus
        self.bus = jcan.Bus()

        # Set filter IDs and callbacks.
        self.bus.set_id_filter([0x4A1, 0x4B1])
        self.bus.add_callback(0x4A1, self.get_callback(0x4A1))
        self.bus.add_callback(0x4B1, self.get_callback(0x4B1))

        #create timers
        self.can_spin_timer = self.create_timer(0.05, self.bus.spin)
        self.publish_data_timer = self.create_timer(1, self.publish_data)
        self.polling_data_timer = self.create_timer(1, self.poll_load_cell)

        # create status
        self.polling_status = False
        self.polling_interval = 20

        # initialise kiln mass message
        self.kiln_id = 0
        self.mass = 0.

        self.last_time = self.get_clock()

        #open the can bus
        self.bus.open(self.get_parameter("canbus").value)


    def get_callback(self, id):
        """
        Returns a callback function for the kilns
        :return:
        """
        def callback(frame):
            if id == 0x4A1:
                # TODO: check frame type? is it list?
                self.mass = convert_to_grams(frame.data[1:])  
                self.kiln_id = 0
                self.get_logger().warning(f"\033[92;1mID: {self.kiln_id}, mass: {self.mass}.\033[0m")

            elif id == 0x4B1:
                self.mass = convert_to_grams(frame.data[1:])
                self.kiln_id = int(frame.data[0])
                self.get_logger().warning(f"\033[92;1mID: {self.kiln_id}, mass: {self.mass}.\033[0m")
                
        return callback
    
    
    def publish_data(self):
        # Publish mass message data.
        msg = KilnMassData()
        msg.id = self.kiln_id
        msg.mass = self.mass
        self.publisher.publish(msg)


    def poll_load_cell(self):
        # Poll only if enabled.
        if self.polling_status:
            now = self.get_clock.now()
            duration: Duration = now - self.last_time
            # Check if the duration has been longer than the interval set by GUI.
            # Purpose: Avoid polling too often.
            if duration >= Duration(self.polling_interval):
                # Biln 1
                self.bus.send(jcan.Frame(0x0B0, [0x0D, 0x01]))
                # Biln 2
                self.bus.send(jcan.Frame(0x0B0, [0x0D, 0x03]))
                # Biln 3
                self.bus.send(jcan.Frame(0x0B0, [0x0D, 0x05]))
                self.last_time = now


    def check_poll_status_callback(self, msg):
        # If polling interval changes, reset timer.
        if self.polling_interval == msg.interval:
            self.last_time = self.get_clock.now()
        self.polling_status = msg.enabled
        self.polling_interval = msg.interval


def main():
    rclpy.init()
    publisher_node = KilnMassDataPublisher()
    rclpy.spin(publisher_node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()