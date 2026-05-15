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
  TX: 0x1F1#XX (XX = channel 0 or 1)
  RX: 0x41C#AA AA BB BB BB BB
      - AAAA: voltage in mV (int16, big-endian)
      - BBBBBBBB: current in µA (int32, big-endian)
  STOP: 0x41C#00 (first byte 0x00 when done)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UNIT CONVERSION:
  CAN mV → Published V (÷1000)
  CAN µA → Published mA (÷1000)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Felicity Matthews
CREATION:       09/05/2026
EDITED:         10/05/2026
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

        self.can_tx_id = self.declare_parameter("can_tx_id", 0x01A, # 01A and 01b are two channels
            ParameterDescriptor(description="CAN ID for sending trigger messages")).value

        self.can_rx_id = self.declare_parameter("can_rx_id", 0x41A, # 41A and 41B are two channels
            ParameterDescriptor(description="CAN ID for receiving voltage/current data")).value

        can_spin_rate = self.declare_parameter("can_spin_rate", 20.0,
            ParameterDescriptor(description="CAN bus polling rate in Hz")).value

        # State
        self.is_receiving = False
        self.active_channel = 0  # Track which channel is currently active

        # CAN bus setup
        self.bus = jcan.Bus()
        self.bus.open(can_bus)
        self.bus.add_callback(self.can_rx_id, self._on_can_receive)
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
            f"Potentiostat node initialized (TX: 0x{self.can_tx_id:03X}, RX: 0x{self.can_rx_id:03X}, rate: {can_spin_rate}Hz)"
        )

    def _trigger_callback(self, request, response):
        """Service callback - send trigger CAN message"""
        option = request.option
        if option not in [0, 1]:
            response.success = False
            response.message = f"Invalid option {option}, must be 0 or 1"
            return response

        frame = jcan.Frame(self.can_tx_id, [option])
        self.bus.send(frame)
        self.is_receiving = True
        self.active_channel = option
        self.get_logger().info(f"Triggered potentiostat on channel {option}")

        response.success = True
        response.message = f"Triggered channel {option}"
        return response

    def _on_can_receive(self, frame: jcan.Frame):
        """CAN callback - parse and publish data"""
        data = frame.data

        # Check for stop alert (first byte == 0x00)
        if data[0] == 0x00:
            self.is_receiving = False
            self.get_logger().info("Potentiostat measurement complete")
            # Publish final message with is_receiving=False
            msg = PotentiostatData()
            msg.header.stamp = self.get_clock().now().to_msg()
            msg.channel = self.active_channel
            msg.voltage = 0.0
            msg.current = 0.0
            msg.is_receiving = False
            self.data_publisher.publish(msg)
            return

        # Parse voltage (bytes 0-3, mV) and current (bytes 4-7, mA)
        voltage_mv = int.from_bytes(data[0:4], 'big', signed=True)
        current_ma = int.from_bytes(data[4:8], 'big', signed=True)

        self.get_logger().info(f"current: {data[4:8]} -> {current_ma} mA, voltage: {data[0:4]} -> {voltage_mv} mV\n {data}")

        # Convert: mV → V
        voltage_v = voltage_mv / 1000.0
        current_ma = current_ma / 1000.0

        # Publish
        msg = PotentiostatData()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.channel = self.active_channel
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
