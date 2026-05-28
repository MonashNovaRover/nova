#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Potentiostat CAN interface node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: potentiostat
TOPICS:
    - publisher: /science/potentiostat/data [PotentiostatData]
SERVICES:
    - service: /science/potentiostat/trigger [TriggerOption]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CAN PROTOCOL:
  TX Ch1: 0x01A# (no data, just trigger)
  TX Ch2: 0x01B# (no data, just trigger)
  RX Ch1: 0x41A#CC CC CC CC VV VV VV VV
  RX Ch2: 0x41B#CC CC CC CC VV VV VV VV
      - CCCCCCCC: current in µA (int32, little-endian)
      - VVVVVVVV: voltage in mV (int32, little-endian)
  STOP: All zeros (8-byte) indicates last message
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UNIT CONVERSION:
  CAN µA → Published mA (÷1000)
  CAN mV → Published V (÷1000)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Felicity Matthews
CREATION:       09/05/2026
EDITED:         22/05/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from rcl_interfaces.msg import ParameterDescriptor
import jcan
from science_interfaces.msg import PotentiostatData
from science_interfaces.srv import TriggerOption


class PotentiostatNode(Node):

    def __init__(self):
        super().__init__("potentiostat")

        # Declare and get parameters
        can_bus = self.declare_parameter("can_bus", "can1",
            ParameterDescriptor(description="CAN interface name")).value

        self.can_trigger_id_ch1 = self.declare_parameter("can_trigger_id_ch1", 0x01A,
            ParameterDescriptor(description="CAN ID for triggering channel 1")).value

        self.can_trigger_id_ch2 = self.declare_parameter("can_trigger_id_ch2", 0x01B,
            ParameterDescriptor(description="CAN ID for triggering channel 2")).value

        self.can_data_id_ch1 = self.declare_parameter("can_data_id_ch1", 0x41A,
            ParameterDescriptor(description="CAN ID for receiving channel 1 data")).value

        self.can_data_id_ch2 = self.declare_parameter("can_data_id_ch2", 0x41B,
            ParameterDescriptor(description="CAN ID for receiving channel 2 data")).value

        can_spin_rate = self.declare_parameter("can_spin_rate", 20.0,
            ParameterDescriptor(description="CAN bus polling rate in Hz")).value

        self.publish_rate = self.declare_parameter("publish_rate", 5.0,
            ParameterDescriptor(description="Max data publish rate in Hz")).value
        self.publish_interval = 1.0 / self.publish_rate if self.publish_rate > 0 else 0

        # State
        self.is_receiving = False
        self.active_channel = 0  # Track which channel is currently active
        self.last_publish_time = 0.0
        self.last_voltage = None
        self.last_current = None

        # CAN bus setup
        self.bus = jcan.Bus()
        self.bus.open(can_bus)
        self.bus.add_callback(self.can_data_id_ch1, lambda f: self._on_can_receive(f, channel=0))
        self.bus.add_callback(self.can_data_id_ch2, lambda f: self._on_can_receive(f, channel=1))
        self.create_timer(1.0 / can_spin_rate, self.bus.spin)

        # ROS interfaces
        self.trigger_service = self.create_service(
            TriggerOption,
            "/science/potentiostat/trigger",
            self._trigger_callback
        )
        self.data_publisher = self.create_publisher(
            PotentiostatData,
            "/science/potentiostat/data",
            10
        )

        self.get_logger().info(
            f"Potentiostat node initialized (Trigger: 0x{self.can_trigger_id_ch1:03X}/0x{self.can_trigger_id_ch2:03X}, "
            f"Data: 0x{self.can_data_id_ch1:03X}/0x{self.can_data_id_ch2:03X}, CAN rate: {can_spin_rate}Hz, "
            f"publish rate: {self.publish_rate}Hz)"
        )

    def _trigger_callback(self, request, response):
        """Service callback - send trigger CAN message to channel-specific ID"""
        channel = request.option
        if channel not in [0, 1]:
            response.success = False
            response.message = f"Invalid channel {channel}, must be 0 or 1"
            return response

        # Select trigger ID based on channel (no data needed, just the trigger)
        trigger_id = self.can_trigger_id_ch1 if channel == 0 else self.can_trigger_id_ch2
        frame = jcan.Frame(trigger_id, [])
        self.bus.send(frame)
        self.is_receiving = True
        self.active_channel = channel
        self.last_voltage = None
        self.last_current = None
        self.get_logger().info(f"Triggered potentiostat channel {channel} (CAN ID: 0x{trigger_id:03X})")

        response.success = True
        response.message = f"Triggered channel {channel}"
        return response

    def _on_can_receive(self, frame: jcan.Frame, channel: int):
        """CAN callback - parse and publish data"""
        data = frame.data

        # Check for stop signal (8-byte all zeros)
        if len(data) == 8 and all(b == 0x00 for b in data):
            self.is_receiving = False
            self.get_logger().info(f"Potentiostat channel {channel} measurement complete")
            # Publish final message with is_receiving=False
            msg = PotentiostatData()
            msg.header.stamp = self.get_clock().now().to_msg()
            msg.channel = channel
            msg.voltage = 0.0
            msg.current = 0.0
            msg.is_receiving = False
            self.data_publisher.publish(msg)
            return

        # Rate limiting check
        current_time = self.get_clock().now().nanoseconds / 1e9
        if current_time - self.last_publish_time < self.publish_interval:
            return  # Skip this message due to rate limiting
        self.last_publish_time = current_time

        # Parse current (bytes 0-3, µA) and voltage (bytes 4-7, mV)
        current_ua = int.from_bytes(data[0:4], 'big', signed=True)
        voltage_mv = int.from_bytes(data[4:8], 'big', signed=True)

        # Convert: µA → mA, mV → V, rounded to 5 decimal places
        current_ma = round(current_ua / 1000.0, 5)
        voltage_v = round(voltage_mv / 1000.0, 5)

        # Skip duplicate readings
        if voltage_v == self.last_voltage and current_ma == self.last_current:
            return
        self.last_voltage = voltage_v
        self.last_current = current_ma

        self.get_logger().debug(
            f"Ch{channel} - current: {current_ua} µA ({current_ma} mA), voltage: {voltage_mv} mV ({voltage_v} V)"
        )

        # Publish
        msg = PotentiostatData()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.channel = channel
        msg.voltage = voltage_v
        msg.current = current_ma
        msg.is_receiving = True
        self.data_publisher.publish(msg)


def main(args=None):
    rclpy.init(args=args)
    node = PotentiostatNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
