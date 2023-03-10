#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: A template ROS node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: path_planner_node
TOPICS:
  - subscriber: /template/subscriber [RoverPose]
  - publisher: /template/publisher [String]
SERVICES:
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Taaj Street
CREATION:	09/03/2023
EDITED:		09/03/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - change all the template artefacts
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
import jcan

# import custom messages
from core.msg import BLCMDReset, BLCMDStatus, BLCMDStatusArray

# import diagnostic_msgs
from diagnostic_msgs.msg import DiagnosticStatus, DiagnosticArray


class BLCMDStatusMonitor(Node):

    def __init__(self):
        super().__init__("BLCMDStatusMonitor")
        #subscriber to reset the blcmd
        self.subscriber = self.create_subscription(BLCMDReset, "/control/blcmd_reset", self.reset_blcmd, 10)
        #publisher to publish the status of the blcmd
        self.publisher = self.create_publisher(BLCMDStatusArray, "/control/blcmd_status", 10)

        #declare parameters
        self.declare_parameter("num_blcmds", 8)
        self.declare_parameter("canbus", "can0")

        #initialise blcmd status array
        self.blcmds_status = []
        for i in range(self.get_parameter("num_blcmds").value):
            self.blcmds_status.append(BLCMDStatus())
            self.blcmds_status[i].id = i + 1
            self.blcmds_status[i].status = BLCMDStatus.OK

        #initialise the can bus
        self.bus = jcan.Bus()
        self.bus.set_id_filter_mask(0x400, 0xF0F)

        #add callbacks for each blcmd
        for i in range(self.get_parameter("num_blcmds").value):
            self.bus.add_callback(0x400 | (i + 1) << 4, self.get_callback(i))

        #create timers
        self.can_spin_timer = self.create_timer(0.01, self.bus.spin)
        self.publish_status_timer = self.create_timer(1/50, self.publish_status)

        #open the can bus
        self.bus.open(self.get_parameter("canbus").value)

    def reset_blcmd(self, msg):
        """
        Updates the classes internal msg state
        :param msg: core.msg.BLCMDReset message from the subscriber callback
        :return: None
        """
        if self.blcmds_status[msg.id -1].status != BLCMDStatus.OK:
            self.bus.send(jcan.Frame(0x40B | msg.id << 4, [0]))
            self.blcmds_status[msg.id -1].status = BLCMDStatus.OK
            self.get_logger().info(f'Reset BLCMD {msg.id}')

    def get_callback(self, blcmd):
        """
        Returns a callback function for the given blcmd
        :param blcmd:
        :return:
        """
        def callback(frame):
            if frame.data[0] == 0:
                if frame.data[1] == 5:
                    self.get_logger().error(f'Gate Driver Fault on BLCMD {blcmd + 1}')
                    self.blcmds_status[blcmd].status = BLCMDStatus.GATE_DRIVER_FAULT
                elif frame.data[1] == 0xA:
                    self.get_logger().error(f'Stall on BLCMD {blcmd + 1}')
                    self.blcmds_status[blcmd].status = BLCMDStatus.STALL_TRIGGERED
        return callback

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
