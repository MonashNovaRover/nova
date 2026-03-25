#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS node to monitor the status of the blcmds
         and receive reset commands
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: blcmd_status_monitor
TOPICS:
  - subscriber: /blcmds/reset [BLCMDReset]
  - publisher: /blcmds/status [BLCMDStatusArray] (TODO: Remove when no longer used)
  - publisher: /drive/blcmd_log [BLCMDLog]
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
from collections.abc import Callable

# import custom messages
from blcmd_interfaces.msg import BLCMDStatus, BLCMDStatusArray, BLCMDLog
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

    # add new blcmd info messages here
    INFO_MESSAGES = {
        0 : "Gate driver config success",
        1 : "MA302 config success",
        2 : "Gate driver fault recovered",
        3 : "Rotor home success",
        4 : "Resolver zero success"
    }

    # no GATE_DRIVE_CONDITION_MESSAGES as that is handled in a special way

    # attributes of BLCMDStatus message with their corresponding error code
    BLCMD_STATUS_ERROR_ATTRS = {
        2 : "resolver_fault",
        5 : "gate_fault",
        10 : "stall_fault",
        11 : "overspeed_fault",
        12 : "overacceleration_fault",
        13 : "encoder_fault",
    }

    def __init__(self):
        super().__init__("blcmd_status_monitor")
        #publisher to publish the status of the blcmd
        self.status_publisher = self.create_publisher(BLCMDStatusArray, "/blcmds/blcmd_status", 10)
        #publisher to publish log messages from each blcmd
        self.log_publisher = self.create_publisher(BLCMDLog, "/drive/blcmd_log", 1)
        #service to reset the blcmd
        self.reset_service = self.create_service(BLCMDReset, "/blcmds/blcmd_reset", self.reset)

        #declare parameters
        self.num_blcmds = self.declare_parameter("num_blcmds", 8).value
        self.declare_parameter("canbus", "can0")

        self.output_period = 1 / int(self.declare_parameter("output_max_frequency", 1).value) # in logs per second
        self.publish_log_period = 1 / int(self.declare_parameter("publish_log_max_frequency", 10).value) # in logs per second
        self.publish_status_period = 1 / int(self.declare_parameter("publish_status_frequency", 2).value) # in blcmd statuses per second

        # disabled by default as there seems to be issues with these messages rn
        self.log_gate_driver_condition = self.declare_parameter("log_gate_driver_condition", False).value

        # params used to determine BLCMDStatus (i.e. keep track of blcmd error states so they can be published)
        self.blcmd_statuses = [BLCMDStatus(id=i+1) for i in range(self.get_parameter("num_blcmds").value)]
        self.error_active_duration = self.declare_parameter("error_active_duration", 2).value # in seconds

        # remember when last output, log published, and when error was last received (used to determine BLCMDStatus)
        self.output_times = {}
        self.publish_log_times = [{} for _ in range(self.num_blcmds)]
        self.error_times = [{} for _ in range(self.num_blcmds)]

        # stores registers temporarily until gate driver fault message sequence is complete
        # see gateDriver.c in blcmd firmware
        self.gate_driver_registers = []

        # parameters for auto blcmd reset feature
        self.enable_auto_blcmd_reset = self.declare_parameter("enable_auto_blcmd_reset", False).value
        self.max_resets = self.declare_parameter("max_resets", 5).value # set to 0 for no limit
        self.reset_timeout = self.declare_parameter("reset_timeout", 5).value # in seconds; set to 0 for no timeout
        self.auto_reset_drive_blmcd_ids = self.declare_parameter("auto_reset_drive_blmcd_ids", [1, 2, 3, 4]).value # which drive blcmds are allowed to be reset (by id)
        if self.auto_reset_drive_blmcd_ids is None:
            self.auto_reset_drive_blmcd_ids = []
        self.auto_reset_pivot_blmcd_ids = self.declare_parameter("auto_reset_pivot_blmcd_ids", []).value # which pivot blcmds are allowed to be reset (by id)
        if self.auto_reset_pivot_blmcd_ids is None:
            self.auto_reset_pivot_blmcd_ids = []

        self.blcmd_zero_response_timeout = self.declare_parameter("blcmd_zero_response_timeout", 3).value # in seconds (only used for pivots)

        # keep track of when blcmd was last reset and total number of resets sent
        self.blcmd_reset_times = {}
        self.blcmd_reset_count = {}

        # keep track of when pivot blcmds were last reset
        self.blcmd_pivot_reset_times = {}

        # list of functions to call after bus.spin()
        # workaround required due to inability to call bus.send in a can callback
        self.deferred_functions: list[Callable[[], None]] = []

        #create reset_time variable to prevent multiple resets in a short time
        self.reset_time = self.get_clock().now()

        #initialise the can bus
        self.bus = jcan.Bus()
        self.bus.set_id_filter_mask(0x400, 0xF00)

        #add callbacks for each blcmd
        for i in range(self.num_blcmds):
            self.bus.add_callback(0x400 | (i + 1) << 4, self.get_callback(i))
            self.bus.add_callback(0x409 | (i + 1) << 4, self.get_reset_pivot_blcmd_callback(i+1))

        #create timers
        self.run_callbacks_timer = self.create_timer(0.01, self.run_callbacks)
        self.publish_status_timer = self.create_timer(self.publish_status_period, self.publish_blcmd_status)

        #open the can bus
        self.bus.open(self.get_parameter("canbus").value)

    def run_callbacks(self):
        self.bus.spin()

        for function in self.deferred_functions:
            function()
        self.deferred_functions = []

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
        except:
            self.get_logger().error('BLCMD Reset or Resolver Reset Failed')
            res.success = False
        return res

    def reset_drive_blcmd(self, blcmd_id: int):
        msg_id = 0x00B | (blcmd_id << 4)

        def deferred_reset():
            self.get_logger().info(f'Resetting drive BLCMD {blcmd_id} due to errors received')
            self.bus.send(
                jcan.Frame(id=msg_id, data=[])
            )

        self.deferred_functions.append(deferred_reset)

    def reset_pivot_blcmd(self, blcmd_id: int):

        # WARNING: This process is based off firmware electrical has written for arm (they should adapt this feature for pivot firmware)

        def deferred_reset():
            self.get_logger().info(f'Resetting pivot BLCMD {blcmd_id} due to errors received (getting current zero position, response not guaranteed)')

            # keep track of when the last request was made
            self.blcmd_pivot_reset_times[blcmd_id] = self.get_clock().now()

            # get configuration (blcmd zero position)
            self.bus.send(
                jcan.Frame(id=0x009 | blcmd_id << 4, data=[0xf])
            )

            # all following operations (including the actual reset of the blcmd occur in callbacks returned by get_reset_pivot_blcmd_callback
            # run by receipt of reply from blcmd containing current zero position via bus.spin (no response = no reset)

        self.deferred_functions.append(deferred_reset)

    def get_reset_pivot_blcmd_callback(self, blcmd_id: int):
        def callback(frame):
            now = self.get_clock().now()

            # don't do anything if there wasn't a recent enough request for pivot zero
            if (blcmd_id not in self.blcmd_pivot_reset_times
              or self.blcmd_pivot_reset_times[blcmd_id] is None
              or now - self.blcmd_pivot_reset_times[blcmd_id] > Duration(seconds=self.blcmd_zero_response_timeout)):
                return

            # don't do anything if this isn't a response containing zero position
            if frame.data[0] != 0xf:
                return

            self.get_logger().info(f'Response received for BLCMD zero position, resetting pivot BLCMD {blcmd_id}')

            zero_position = frame.data[1:3]

            def deferred_reset():

                # reset pivot blcmd
                self.bus.send(
                    jcan.Frame(id=0x00B | (blcmd_id << 4), data=[])
                )

                # set pivot zero
                self.bus.send(
                    jcan.Frame(id=0x00A | (blcmd_id << 4), data=[
                        0xf,
                        *zero_position
                    ])
                )

                self.blcmd_pivot_reset_times[blcmd_id] = None

            self.deferred_functions.append(deferred_reset)

        return callback


    # only run when an error from blcmd is detected
    def auto_blcmd_reset(self, blcmd_id: int):
        # check if auto blcmd reset is disabled
        if not self.enable_auto_blcmd_reset:
            return

        # check if this is a blcmd id we can reset
        if (blcmd_id not in self.auto_reset_drive_blmcd_ids
          and blcmd_id not in self.auto_reset_pivot_blmcd_ids):
            return

        now = self.get_clock().now()

        # check if it's too soon to send another reset (in timeout)
        if (self.reset_timeout > 0
          and blcmd_id in self.blcmd_reset_times
          and (now - self.blcmd_reset_times[blcmd_id]) < Duration(seconds=self.reset_timeout)):
            return

        # check if we can't send more (reached max_resets)
        if (self.max_resets > 0
          and blcmd_id in self.blcmd_reset_count
          and self.blcmd_reset_count[blcmd_id] >= self.max_resets):
            return

        # all checks passed, send reset now

        # update counts and times
        self.blcmd_reset_times[blcmd_id] = now
        if blcmd_id not in self.blcmd_reset_count:
            self.blcmd_reset_count[blcmd_id] = 1
        else:
            self.blcmd_reset_count[blcmd_id] += 1


        if blcmd_id in self.auto_reset_pivot_blmcd_ids:
            self.reset_pivot_blcmd(blcmd_id)
        else:
            self.reset_drive_blcmd(blcmd_id)

    def output_rate_limited(self, blcmd: int, output_message: str) -> bool:
        output_id = (blcmd, output_message)

        if (output_id not in self.output_times
          or (self.get_clock().now() - self.output_times[output_id]) >= Duration(seconds=self.output_period)):
            self.output_times[output_id] = self.get_clock().now()

            # is not rate limited
            return False

        # is rate limited
        return True

    def publish_log(self, blcmd: int, type: int, log_message: list[int]) -> None:
        publish_times = self.publish_log_times[blcmd]

        log_id = (type, *log_message)
        if (log_id not in publish_times
          or (self.get_clock().now() - publish_times[log_id]) >= Duration(seconds=self.publish_log_period)):
            publish_times[log_id] = self.get_clock().now()

            # not rate limited, so publish
            msg = BLCMDLog()
            msg.id = blcmd + 1
            msg.type = type
            msg.message = log_message
            self.log_publisher.publish(msg)

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

                # publish blcmd log
                self.publish_log(blcmd, frame.data[0], [frame.data[1]])

                # update blcmd status with received error
                blcmd_status = self.blcmd_statuses[blcmd]

                error_id = frame.data[1]
                if (error_id in self.BLCMD_STATUS_ERROR_ATTRS
                  and hasattr(blcmd_status, self.BLCMD_STATUS_ERROR_ATTRS[error_id])):
                    setattr(blcmd_status, self.BLCMD_STATUS_ERROR_ATTRS[error_id], True)
                    self.error_times[blcmd][error_id] = self.get_clock().now()

                # attempt to reset blcmd automatically
                self.auto_blcmd_reset(blcmd + 1)

            # warnings
            elif frame.data[0] == 1:
                if frame.data[1] in self.WARNING_MESSAGES:
                    warning_message = self.WARNING_MESSAGES[frame.data[1]]
                else:
                    warning_message = f"Unknown warning \"{frame.data[1]:#2x}\""

                if not self.output_rate_limited(blcmd, warning_message):
                    self.get_logger().warning(f"{warning_message} on {source}")

                # publish blcmd log
                self.publish_log(blcmd, frame.data[0], [frame.data[1]])

            # info
            elif frame.data[0] == 2:
                if frame.data[1] in self.INFO_MESSAGES:
                    info_message = self.INFO_MESSAGES[frame.data[1]]
                else:
                    info_message = f"Unknown info \"{frame.data[1]:#2x}\""

                if not self.output_rate_limited(blcmd, info_message):
                    self.get_logger().info(f"{info_message} on {source}")

                # publish blcmd log
                self.publish_log(blcmd, frame.data[0], [frame.data[1]])

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

                    # publish blcmd log
                    self.publish_log(blcmd, frame.data[0], self.gate_driver_registers)

                    # reset for next sequence
                    self.gate_driver_registers = []

                # received message containing id of faulted register
                else:
                    self.gate_driver_registers.append(data)

            # any log messages with unknown severity
            else:
                log_data = frame.data[1:]
                error_message = f"Unknown severity \"{frame.data[0]:#2x}\" with remaining data \"{log_data}\""
                if not self.output_rate_limited(blcmd, error_message):
                    self.get_logger().error(f"{error_message} on {source}")

                # publish blcmd log
                self.publish_log(blcmd, frame.data[0], log_data)

        return callback

    def publish_blcmd_status(self):
        now = self.get_clock().now()

        # remove stale errors
        for i, blcmd_status in enumerate(self.blcmd_statuses):
            for error_id, error_time in self.error_times[i].items():
                if (now - error_time) > Duration(seconds=self.error_active_duration):
                    setattr(blcmd_status, self.BLCMD_STATUS_ERROR_ATTRS[error_id], False)

        msg = BLCMDStatusArray()
        msg.blcmds = self.blcmd_statuses
        self.status_publisher.publish(msg)

def main():
    rclpy.init()
    monitor_node = BLCMDStatusMonitor()
    rclpy.spin(monitor_node)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
