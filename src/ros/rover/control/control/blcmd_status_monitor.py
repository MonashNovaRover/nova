#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS node to monitor the status of the blcmds
         and receive reset commands
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: blcmd_status_monitor
TOPICS:
  - subscriber: /control/blcmd_reset [BLCMDReset]
  - publisher: /control/blcmd_status [BLCMDStatusArray]
SERVICES:
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Taaj Street
CREATION:	09/03/2023
EDITED:		15/03/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from rclpy.time import Time
from rclpy.duration import Duration
import jcan

# import custom messages
from core.msg import BLCMDReset, BLCMDStatus, BLCMDStatusArray

class BLCMDStatusMonitor(Node):

    def __init__(self):
        super().__init__("blcmd_status_monitor")
        #subscriber to reset the blcmd
        self.subscriber = self.create_subscription(BLCMDReset, "/control/blcmd_reset", self.reset_blcmd, 10)
        #publisher to publish the status of the blcmd
        self.publisher = self.create_publisher(BLCMDStatusArray, "/control/blcmd_status", 10)

        #declare parameters
        self.declare_parameter("num_blcmds", 8)
        self.declare_parameter("canbus", "can0")

        #initialise blcmd status array and fault times dict
        self.blcmds_status = []
        self.fault_times = {}
        for i in range(self.get_parameter("num_blcmds").value):
            self.blcmds_status.append(BLCMDStatus())
            self.blcmds_status[i].id = i + 1
            self.blcmds_status[i].gate_fault = False
            self.blcmds_status[i].stall_fault = False
            self.blcmds_status[i].resolver_fault = False

        #create reset_time variable to prevent multiple resets in a short time
        self.reset_time = self.get_clock().now()

        #initialise the can bus
        self.bus = jcan.Bus()
        self.bus.set_id_filter_mask(0x400, 0xF0F)

        #add callbacks for each blcmd
        for i in range(self.get_parameter("num_blcmds").value):
            self.bus.add_callback(0x400 | (i + 1) << 4, self.get_callback(i))

        #create timers
        self.can_spin_timer = self.create_timer(0.01, self.bus.spin)
        self.publish_status_timer = self.create_timer(1/50, self.publish_status)
        self.status_check_timer = self.create_timer(1, self.check_status)

        #open the can bus
        self.bus.open(self.get_parameter("canbus").value)

    def reset_blcmd(self, msg):
        """
        Updates the classes internal msg state
        :param msg: core.msg.BLCMDReset message from the subscriber callback
        :return: None
        """
        self.bus.send(jcan.Frame(0x00B | msg.id << 4, [0]))
        self.get_logger().info(f'Reset BLCMD {msg.id}')

    def get_callback(self, blcmd):
        """
        Returns a callback function for the given blcmd
        :param blcmd:
        :return:
        """
        def callback(frame):
            if frame.data[0] == 0:
                if frame.data[1] == 0x2:
                    self.get_logger().error(f'Resolver Fault on BLCMD {blcmd + 1}')
                    self.blcmds_status[blcmd].resolver_fault = True
                    self.fault_times["resolver_fault"] = self.get_clock().now()
                elif frame.data[1] == 0x5:
                    self.get_logger().error(f'Gate Driver Fault on BLCMD {blcmd + 1}')
                    self.blcmds_status[blcmd].gate_fault = True
                    self.fault_times["gate_fault"] = self.get_clock().now()
                elif frame.data[1] == 0xA:
                    self.get_logger().error(f'Stall Fault on BLCMD {blcmd + 1}')
                    self.blcmds_status[blcmd].stall_fault = True
                    self.fault_times["stall_fault"] = self.get_clock().now()
                elif frame.data[1] == 0xB:
                    self.get_logger().error(f'Over Speed Fault on BLCMD {blcmd + 1}')
                    self.blcmds_status[blcmd].overspeed_fault = True
                    self.fault_times["overspeed_fault"] = self.get_clock().now()
        return callback

    def check_status(self):
        for i in range(self.get_parameter("num_blcmds").value):
            if self.blcmds_status[i].resolver_fault:
                if (self.get_clock().now() - self.fault_times["resolver_fault"]) > Duration(seconds=2):
                    self.blcmds_status[i].resolver_fault = False
            if self.blcmds_status[i].gate_fault:
                if (self.get_clock().now() - self.fault_times["gate_fault"]) > Duration(seconds=2):
                    self.blcmds_status[i].gate_fault = False
            if self.blcmds_status[i].stall_fault:
                if (self.get_clock().now() - self.fault_times["stall_fault"]) > Duration(seconds=2):
                    self.blcmds_status[i].stall_fault = False
            if self.blcmds_status[i].overspeed_fault:
                if (self.get_clock().now() - self.fault_times["overspeed_fault"]) > Duration(seconds=2):
                    self.blcmds_status[i].resolver_fault = False

    def publish_status(self):
        """
        Publishes the status of the blcmds
        :return: None
        """
        msg = BLCMDStatusArray()
        msg.blcmds = self.blcmds_status
        self.publisher.publish(msg)


def main():
    rclpy.init()
    monitor_node = BLCMDStatusMonitor()
    rclpy.spin(monitor_node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
