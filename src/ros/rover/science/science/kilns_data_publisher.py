#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 receiver code for the kilns and bilns data publisher.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ PACKAGE:     electronics
AUTHOR(S):   Niko Verrios
CREATION:    18/03/2023
EDITED:      22/03/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import math
import rclpy
from rclpy.node import Node
from rclpy.duration import Duration
import jcan

# import custom messages
from core.msg import KilnMassData, KilnTempData
from core.srv import LoadCellPoller

# Constants for temperature conversion.
SERIES_RESISTOR = 100000    # Might change.
R_0 = 10000                 # Resistance value (10K resistor)
T_0 = 25                    # Outside temp in K
B_COEFFICIENT = 3950        # Dependent on series resistor
KELV = 273.15


def convert_to_grams(data):
    return int.from_bytes(bytes(data), "big", signed=True)/1000


def convert_to_celcius(data):
    byte_int = int.from_bytes(bytes(data), "big", signed=False)
    analog = SERIES_RESISTOR / (1023 / byte_int - 1)
    steinhart = math.log(analog / R_0) / B_COEFFICIENT + (1 / (T_0 + KELV))
    return 1 / steinhart - KELV


class KilnMassDataPublisher(Node):

    def __init__(self):
        super().__init__("kiln_data_publisher")


        # Print initialisation information
        self.get_logger().warning("\033[92;1mInitialising the Kiln Mass Publisher class.\033[0m")

        #subscriber to polling status
        self.service = self.create_service(LoadCellPoller, '/science/load_cell_poller', self.load_cell_callback_func)
        #publisher to publish the data from the kilns.
        self.mass_publisher = self.create_publisher(KilnMassData, "/science/kiln_mass_data", 1)
        self.temp_publisher = self.create_publisher(KilnTempData, "/science/kiln_temp_data", 1)

        #declare parameters
        self.declare_parameter("canbus", "can1")

        #initialise the can bus
        self.bus = jcan.Bus()

        # Set filter IDs and callbacks.
        self.bus.set_id_filter([0x4B1, 0x4C3])
        self.bus.add_callback(0x4B1, self.get_mass_callback(0x4B1))
        self.bus.add_callback(0x4C3, self.get_temp_callback())  

        #create timers
        self.can_spin_timer = self.create_timer(0.05, self.bus.spin)
        self.publish_data_timer = self.create_timer(1, self.publish_data)
        self.polling_data_timer = self.create_timer(1, self.poll_load_cell)

        # create status
        self.polling_status = False
        self.polling_interval = 20

        # initialise kiln data 
        self.masses = [0., 0., 0.]
        self.temps = [0., 0., 0., 0.]

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
            try:
                id = int(frame.data[0])
                self.masses[id] = convert_to_grams(frame.data[1:])
                self.get_logger().info(f"\033[92;1mMass Data packet received from canbus.\033[0m")
            except Exception as e:
                self.get_logger().error(f"\033[91;1mMass data packet failed and threw an error: {e}\033[0m")
                
        return callback
    

    def get_temp_callback(self):
        """
        Returns a callback function for the thermistors
        :return:
        """
        def callback(frame):
            # Data returned is a byte list. First byte is the kiln ID, other bytes is the remaining data.
            try:
                id = int(frame.data[0])
                self.temps[id-1] = convert_to_celcius(frame.data[1:])
            except Exception as e:
                self.get_logger().error(f"\033[91;1mMass data packet failed and threw an error: {e}\033[0m")
            
        return callback
    
    
    def publish_data(self):
        # Publish mass message data.
        if self.polled:
            for i in range(len(self.masses)):
                msg = KilnMassData()
                msg.id = i
                msg.mass = self.masses[i]
                self.mass_publisher.publish(msg)
                self.polled = False

        msg = KilnTempData()
        for i in range(len(self.temps)):
            msg.id = i + 1
            msg.temperature = self.temps[i]
            self.temp_publisher.publish(msg)


    def poll_load_cell(self, ex_now=False):
        # Poll only if enabled.
        if self.polling_status:
            now = self.get_clock().now()
            duration: Duration = now - self.last_time
            poll_interval: Duration = Duration(seconds=self.polling_interval)
            # Check if the duration has been longer than the interval set by GUI.
            # Purpose: Avoid polling too often.
            if ex_now or duration >= poll_interval:
                self.get_logger().info("\033[92;1mPolling load cell.\033[0m")
                # Biln 1
                self.bus.send(jcan.Frame(0x0B0, [0x0D, 0x01]))
                # Biln 2
                self.bus.send(jcan.Frame(0x0B0, [0x0D, 0x03]))
                # Biln 3
                self.bus.send(jcan.Frame(0x0B0, [0x0D, 0x05]))
                self.last_time = now
                self.polled = True


    def load_cell_callback_func(self, request: LoadCellPoller.Request(), response: LoadCellPoller.Response()):
        try:
            self.polling_status = request.enabled
            self.polling_interval = request.interval

            response.success = True            
            self.poll_load_cell(ex_now=True)
        
        # If an error occurred
        except Exception as e:
            self.get_logger().error("\033[1;91m\nLoad Cell Poll ERROR! Exception:\n\t%s\033[0m" % e)
            
            # Process failed
            response.success = False

        # Return the response data
        return response


def main():
    rclpy.init()
    publisher_node = KilnMassDataPublisher()
    rclpy.spin(publisher_node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()