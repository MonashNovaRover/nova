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


def convert_to_celcius(data):
    pass


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
            if frame.data[0] == 0:
                if frame.data[1] == 0x5:
                    self.get_logger().error(f'Gate Driver Fault on BLCMD {kiln + 1}')
                    self.blcmds_status[kiln].gate_fault = True
                    self.fault_times["gate_fault"] = self.get_clock().now()
                elif frame.data[1] == 0xA:
                    self.get_logger().error(f'Stall Fault on BLCMD {kiln + 1}')
                    self.blcmds_status[kiln].stall_fault = True
                    self.blcmds_status[kiln].stall_fault = True
                    self.fault_times["stall_fault"] = self.get_clock().now()
                elif frame.data[1] == 0x2:
                    self.get_logger().error(f'Resolver Fault on BLCMD {kiln + 1}')
                    self.blcmds_status[kiln].resolver_fault = True
                    self.fault_times["resolver_fault"] = self.get_clock().now()
        return callback


def main():
    rclpy.init()
    publisher_node = KilnDataPublisher()
    rclpy.spin(publisher_node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()