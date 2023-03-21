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
import math
import rclpy
from rclpy.node import Node
from rclpy.duration import Duration
import jcan

# import custom messages
from core.msg import KilnMassData, KilnMassPollingStatus, KilnTempData

# Constants for temperature conversion.
SERIES_RESISTOR = 10000     # Might change.
R_0 = 10000                 # Resistance value (10K resistor)
T_0 = 298                   # Outside temp in K
B_COEFFICIENT = 3950        # Dependent on series resistor


def convert_to_grams(data):
    return int.from_bytes(bytes(data), "big", signed=True)/1000

def convert_to_celcius(data):
    analog = SERIES_RESISTOR / (1023 / int.from_bytes(bytes(data), "big", signed=True) - 1)
    steinhart = math.log(analog / R_0) / B_COEFFICIENT + (1 / T_0)
    return 1 / steinhart - 273.15


class KilnMassDataPublisher(Node):

    def __init__(self):
        super().__init__("kiln_data_publisher")


        # Print initialisation information
        self.get_logger().warning("\033[92;1mInitialising the Kiln Mass Publisher class.\033[0m")

        #subscriber to polling status
        self.subscriber = self.create_subscription(KilnMassPollingStatus, "/science/kiln_mass_polling_status", self.check_poll_status_callback, 10)
        #publisher to publish the data from the kilns.
        self.mass_publisher = self.create_publisher(KilnMassData, "/science/kiln_mass_data", 1)
        self.temp_publisher = self.create_publisher(KilnTempData, "/science/kiln_temp_data", 1)

        #declare parameters
        self.declare_parameter("canbus", "can1")

        #initialise the can bus
        self.bus = jcan.Bus()

        # Set filter IDs and callbacks.
        self.bus.set_id_filter([0x4A1, 0x4B1, 0x4C3])
        self.bus.add_callback(0x4A1, self.get_mass_callback(0x4A1))
        self.bus.add_callback(0x4B1, self.get_mass_callback(0x4B1))
        self.bus.add_callback(0x4A1, self.get_temp_callback(0x4C3))  

        #create timers
        self.can_spin_timer = self.create_timer(0.05, self.bus.spin)
        self.publish_data_timer = self.create_timer(1, self.publish_data)
        self.polling_data_timer = self.create_timer(1, self.poll_load_cell)

        # create status
        self.polling_status = True
        self.polling_interval = 20

        # create starting time to deduct
        self.start_time = self.get_clock().now()

        # initialise kiln mass message
        self.mass_kiln_id = 0
        self.mass = 0.
        self.data_interval = 0
        self.temp_kiln_id = 0
        self.temp = 0.

        # polling times set
        self.last_time = self.get_clock().now()
        self.polled = False

        #open the can bus
        self.bus.open(self.get_parameter("canbus").value)


    def get_mass_callback(self, id):
        """
        Returns a callback function for the load cell
        :return:
        """
        def callback(frame):
            # Data returned is a byte list. First byte is the kiln ID, other bytes is the remaining data.
            if id == 0x4A1:
                self.mass = convert_to_grams(frame.data[1:])  
                self.mass_kiln_id = 0
                self.data_interval = int((self.get_clock().now() - self.start_time).nanoseconds/1e9)

            elif id == 0x4B1:
                self.mass = convert_to_grams(frame.data[1:])
                self.mass_kiln_id = int(frame.data[0])
                self.data_interval = int((self.get_clock().now() - self.start_time).nanoseconds/1e9)
                self.get_logger().info(f"\033[92;1mMass Data packet received from canbus.\033[0m")
                
        return callback
    

    def get_temp_callback(self, id):
        """
        Returns a callback function for the thermistors
        :return:
        """
        def callback(frame):
            # Data returned is a byte list. First byte is the kiln ID, other bytes is the remaining data.
            if id == 0x4C3:
                self.temp = convert_to_celcius(frame.data[1:])
                self.temp_kiln_id = int(frame.data[0])
                self.get_logger().info(f"\033[92;1mTemp Data packet received from canbus.\033[0m")
                
        return callback
    
    
    def publish_data(self):
        # Publish mass message data.
        if self.polled:
            msg = KilnMassData()
            msg.id = self.mass_kiln_id
            msg.mass = self.mass
            msg.interval = self.data_interval

            self.mass_publisher.publish(msg)
            self.polled = False

        msg = KilnTempData()
        msg.id = self.temp_kiln_id
        msg.temperature = self.temp
        self.temp_publisher.publish(msg)


    def poll_load_cell(self):
        # Poll only if enabled.
        if self.polling_status:
            now = self.get_clock().now()
            duration: Duration = now - self.last_time
            poll_interval: Duration = Duration(seconds=self.polling_interval)
            # Check if the duration has been longer than the interval set by GUI.
            # Purpose: Avoid polling too often.
            if duration >= poll_interval:
                self.get_logger().info("\033[92;1mPolling load cell.\033[0m")
                # Biln 1
                self.bus.send(jcan.Frame(0x0B0, [0x0D, 0x01]))
                # Biln 2
                self.bus.send(jcan.Frame(0x0B0, [0x0D, 0x03]))
                # Biln 3
                self.bus.send(jcan.Frame(0x0B0, [0x0D, 0x05]))
                self.last_time = now
                self.polled = True


    def check_poll_status_callback(self, msg: KilnMassPollingStatus):
        # If polling interval changes, reset timer.
        if self.polling_interval != msg.interval:
            self.last_time = self.get_clock().now()

        # If enabling graph, set the start time for deduction.
        if msg.enabled and not self.polling_status:
            self.start_time = self.get_clock().now()
        
        self.polling_status = msg.enabled
        self.polling_interval = msg.interval


def main():
    rclpy.init()
    publisher_node = KilnMassDataPublisher()
    rclpy.spin(publisher_node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()