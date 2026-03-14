#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS node to monitor the status of the blcmds
         and receive reset commands
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: blcmd_status_monitor
TOPICS:
  - subscriber: /blcmds/reset [BLCMDReset]
  - publisher: /drive/log [Log]
SERVICES:
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Taaj Street, Kabilan, Jonathan Jia
CREATION:	09/03/2023
EDITED:		14/03/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from rclpy.time import Time
from rclpy.duration import Duration
import jcan

# import custom messages
from nova_interfaces.msg import Log
from blcmd_interfaces.srv import BLCMDReset


class BLCMDStatusMonitor(Node):

    # add new blcmd errors here
    ERROR_MESSAGES = {
        0 : "No steps error",
        1 : "Magnetic encoder error",
        2 : "Resolver error",
        3 : "Resolver failed checksum",
        4 : "Unexpected position command",
        5 : "Gate driver fault",
        6 : "Gate driver SPI failed",
        7 : "MA302 SPI failed",
        8 : "Rotor home failed",
        9 : "Resolver zero failure",
        10 : "Stall triggered",
        11 : "Over speed",
        12 : "Over acceleration",
        13 : "Encoder fault"
    }

    # add new blcmd warnings here
    WARNING_MESSAGES = {
        0 : "Missing steps warning",
        1 : "Duty cycle threshold warning",
        2 : "Current threshold warning",
        3 : "Velocity threshold warning",
        4 : "Position out of range warning"
    }

    # add new blcmd errors here
    INFO_MESSAGES = {
        0 : "Gate driver config success",
        1 : "MA302 config success",
        2 : "Gate driver fault recovered",
        3 : "Rotor home success",
        4 : "Resolver zero success"
    }

    # no GATE_DRIVE_CONDITION_MESSAGES as that is handled in a special way

    def __init__(self):
        super().__init__("blcmd_status_monitor")
        #publisher to publish the status of the blcmd
        self.publisher = self.create_publisher(Log, "/drive/log", 1)
        #service to reset the blcmd
        self.reset_service = self.create_service(BLCMDReset, "/blcmds/blcmd_reset", self.reset)

        #declare parameters
        self.declare_parameter("num_blcmds", 8)
        self.declare_parameter("canbus", "can0")
        self.output_period = 1 / int(self.declare_parameter("output_rate_limit", 1).value) # in logs per second
        self.publish_period = 1 / int(self.declare_parameter("publish_rate_limit", 10).value) # in logs per second

        # disabled by default as there seems to be issues with these messages rn
        self.log_gate_driver_condition = self.declare_parameter("log_gate_driver_condition", False).value

        # remember when last output/published for rate limiting
        self.output_times = [{} for _ in range(self.get_parameter("num_blcmds").value)]
        self.publish_times = [{} for _ in range(self.get_parameter("num_blcmds").value)]

        # stores registers temporarily until all gate driver fault message sequence is complete
        # see gateDriver.c in blcmd firmware
        self.gate_driver_registers = []

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

        #open the can bus
        self.bus.open(self.get_parameter("canbus").value)

    def reset(self,req,res):
        """
        Updates the classes internal msg state
        :param msg: nova_interfaces.msg.BLCMDReset message from the subscriber callback
        :return: None
        """
        try:
            if req.type == BLCMDReset.Request.BLCMD:
                frame = jcan.Frame(0x00B | req.id << 4, [0])
            elif req.type == BLCMDReset.Request.RESOLVER:
                frame = jcan.Frame(0x00C | req.id << 4, [0])

            self.bus.send(frame)
            if req.type == BLCMDReset.Request.BLCMD:
                self.get_logger().info(f'Reset BLCMD {req.id}')
            elif req.type == BLCMDReset.Request.RESOLVER:
                self.get_logger().info(f'Reset resolver on BLCMD {req.id}')
            res.success = True
        except :
            self.get_logger().error('BLCMD Reset or Resolver Reset Failed');
            res.success = False
        return res

    def output_rate_limited(self, blcmd: int, log_message: str) -> bool:
        blcmd_output_times = self.output_times[blcmd]

        if (log_message not in blcmd_output_times
          or (self.get_clock().now() - blcmd_output_times[log_message]) >= Duration(seconds=self.output_period)):
            blcmd_output_times[log_message] = self.get_clock().now()

            # is not rate limited
            return False

        # is rate limited
        return True

    def publish_rate_limited(self, blcmd: int, log_message: str) -> bool:
        blcmd_publish_times = self.publish_times[blcmd]

        if (log_message not in blcmd_publish_times
                or (self.get_clock().now() - blcmd_publish_times[log_message]) >= Duration(seconds=self.publish_period)):
            blcmd_publish_times[log_message] = self.get_clock().now()

            # is not rate limited
            return False

        # is rate limited
        return True

    def get_callback(self, blcmd: int):
        """
        Returns a callback function for the given blcmd
        :param blcmd:
        :return:
        """
        def callback(frame):
            source = f"BLCMD {blcmd + 1}"

            # errors
            if frame.data[0] == 0:
                if frame.data[1] in self.ERROR_MESSAGES:
                    error_message = self.ERROR_MESSAGES[frame.data[1]]
                else:
                    error_message = f"Unknown error \"{frame.data[1]:#2x}\""

                if not self.output_rate_limited(blcmd, error_message):
                    self.get_logger().error(f"{error_message} on {source}")
                if not self.publish_rate_limited(blcmd, error_message):
                    msg = Log()
                    msg.source = source
                    msg.errors = [error_message]
                    self.publisher.publish(msg)

            # warnings
            elif frame.data[0] == 1:
                if frame.data[1] in self.WARNING_MESSAGES:
                    warning_message = self.WARNING_MESSAGES[frame.data[1]]
                else:
                    warning_message = f"Unknown warning \"{frame.data[1]:#2x}\""

                if not self.output_rate_limited(blcmd, warning_message):
                    self.get_logger().warning(f"{warning_message} on {source}")
                if not self.publish_rate_limited(blcmd, warning_message):
                    msg = Log()
                    msg.source = source
                    msg.warnings = [warning_message]
                    self.publisher.publish(msg)

            # info
            elif frame.data[0] == 2:
                if frame.data[1] in self.INFO_MESSAGES:
                    info_message = self.INFO_MESSAGES[frame.data[1]]
                else:
                    info_message = f"Unknown info \"{frame.data[1]:#2x}\""

                if not self.output_rate_limited(blcmd, info_message):
                    self.get_logger().info(f"{info_message} on {source}")
                if not self.publish_rate_limited(blcmd, info_message):
                    msg = Log()
                    msg.source = source
                    msg.info = [info_message]
                    self.publisher.publish(msg)

            # gate driver condition
            elif frame.data[0] == 3:

                # don't log if not enabled
                if not self.log_gate_driver_condition:
                    return

                data = frame.data[1]

                # message sequence ended; output/publish log
                if data == 0xFF:
                    if len(self.gate_driver_registers) > 0:
                        gate_driver_condition_messsage = f"Gate driver fault on registers {self.gate_driver_registers}"
                    else:
                        gate_driver_condition_messsage = f"Gate driver fault (but no registers were received)"

                    if not self.output_rate_limited(blcmd, gate_driver_condition_messsage):
                        self.get_logger().error(f"{gate_driver_condition_messsage} on {source}")
                    if not self.publish_rate_limited(blcmd, gate_driver_condition_messsage):
                        msg = Log()
                        msg.source = source
                        msg.errors = [gate_driver_condition_messsage]
                        self.publisher.publish(msg)

                    # reset for next sequence
                    self.gate_driver_registers = []

                # received message containing id of faulted register
                else:
                    self.gate_driver_registers.append(data)

            # any log messages with unknown severity
            else:
                error_message = f"Unknown severity \"{frame.data[0]:#2x}\" with log \"{frame.data[1]:#2x}\""
                if not self.output_rate_limited(blcmd, error_message):
                    self.get_logger().error(f"{error_message} on {source}")
                if not self.publish_rate_limited(blcmd, error_message):
                    msg = Log()
                    msg.source = source
                    msg.errors = [error_message]
                    self.publisher.publish(msg)

        return callback

def main():
    rclpy.init()
    monitor_node = BLCMDStatusMonitor()
    rclpy.spin(monitor_node)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
