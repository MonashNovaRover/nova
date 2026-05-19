#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Mock Potentiostat hardware for testing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: potentiostat_mock
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Simulates potentiostat hardware by:
  1. Listening for trigger commands on CAN
  2. Reading voltage/current data from CSV
  3. Sending data back on CAN at configurable rate
  4. Sending stop signal when complete
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CAN PROTOCOL:
  RX: 0x01A#XX (trigger, XX = channel 0 or 1)
  TX: 0x41A#VV VV VV VV CC CC CC CC (voltage, current)
      - VVVVVVVV: voltage in 10µV units (int32, little-endian)
      - CCCCCCCC: current in µA (int32, little-endian)
  STOP: 0x41A#00 00 00 00 00 00 00 00 (8-byte all zeros when done)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PARAMETERS:
  can_bus: CAN interface name (default: "can1")
  can_tx_id: CAN ID for sending data (default: 0x41A)
  can_rx_id: CAN ID for receiving trigger (default: 0x01A)
  csv_file: Path to CSV file with voltage,current columns
  send_rate: Rate to send data in Hz (default: 10.0)
  can_spin_rate: CAN bus polling rate in Hz (default: 20.0)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CSV FORMAT (values in 10µV and µA, matching CAN protocol):
  voltage,current
  100000,50000  (= 1.0V, 50mA)
  ...
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import csv
import os
import rclpy
from rclpy.node import Node
from rcl_interfaces.msg import ParameterDescriptor
import jcan

# Default CSV file path (same directory as this script)
DEFAULT_CSV = "/home/nova/nova/src/ros/rover/science/science/science/urc/potentiostat_sample.csv"


class PotentiostatMockNode(Node):

    def __init__(self):
        super().__init__("potentiostat_mock")

        # Declare and get parameters
        can_bus = self.declare_parameter(
            "can_bus", "can1",
            ParameterDescriptor(description="CAN interface name")
        ).value

        self.can_tx_id = self.declare_parameter(
            "can_tx_id", 0x41A,
            ParameterDescriptor(description="CAN ID for sending voltage/current data")
        ).value

        self.can_rx_id = self.declare_parameter(
            "can_rx_id", 0x01A,
            ParameterDescriptor(description="CAN ID for receiving trigger messages")
        ).value

        csv_file = self.declare_parameter(
            "csv_file", DEFAULT_CSV,
            ParameterDescriptor(description="Path to CSV file with voltage,current data")
        ).value

        send_rate = self.declare_parameter(
            "send_rate", 10.0,
            ParameterDescriptor(description="Rate to send data in Hz")
        ).value

        can_spin_rate = self.declare_parameter(
            "can_spin_rate", 20.0,
            ParameterDescriptor(description="CAN bus polling rate in Hz")
        ).value

        # Load CSV data
        self.data = []
        self._load_csv(csv_file)

        # State
        self.is_sending = False
        self.data_index = 0
        self.current_channel = 0

        # CAN bus setup
        self.bus = jcan.Bus()
        self.bus.open(can_bus)
        self.bus.add_callback(self.can_rx_id, self._on_trigger_receive)
        self.create_timer(1.0 / can_spin_rate, self.bus.spin)

        # Data send timer (starts disabled)
        self.send_period = 1.0 / send_rate
        self.send_timer = self.create_timer(self.send_period, self._send_data)
        self.send_timer.cancel()

        self.get_logger().info(
            f"Potentiostat mock initialized (TX: 0x{self.can_tx_id:03X}, RX: 0x{self.can_rx_id:03X}, "
            f"rate: {send_rate}Hz, {len(self.data)} data points)"
        )

    def _load_csv(self, filepath: str):
        """Load voltage/current data from CSV file"""
        try:
            with open(filepath, 'r') as f:
                reader = csv.DictReader(f)
                for row in reader:
                    voltage = int(row['voltage'])
                    current = int(row['current'])
                    self.data.append((voltage, current))
            self.get_logger().info(f"Loaded {len(self.data)} data points from {filepath}")
        except FileNotFoundError:
            self.get_logger().error(f"CSV file not found: {filepath}")
        except KeyError as e:
            self.get_logger().error(f"CSV missing required column: {e}")
        except ValueError as e:
            self.get_logger().error(f"CSV parse error: {e}")

    def _on_trigger_receive(self, frame: jcan.Frame):
        """CAN callback - handle trigger command"""
        if len(frame.data) < 1:
            return

        channel = frame.data[0]
        self.get_logger().info(f"Received trigger for channel {channel}")

        # Start sending data
        self.current_channel = channel
        self.data_index = 0
        self.is_sending = True
        self.send_timer.reset()

    def _send_data(self):
        """Timer callback - send next data point"""
        if not self.is_sending:
            self.send_timer.cancel()
            return

        if self.data_index >= len(self.data):
            # Send stop signal
            self._send_stop()
            return

        voltage, current = self.data[self.data_index]
        self._send_reading(voltage, current)
        self.data_index += 1

    def _send_reading(self, voltage: int, current: int):
        """Send voltage/current reading over CAN"""
        # Pack voltage as 4 bytes (little-endian, signed, in 10µV units)
        voltage_bytes = voltage.to_bytes(4, 'little', signed=True)
        # Pack current as 4 bytes (little-endian, signed, in µA)
        current_bytes = current.to_bytes(4, 'little', signed=True)

        data = list(voltage_bytes) + list(current_bytes)
        frame = jcan.Frame(self.can_tx_id, data)
        self.bus.send(frame)

        self.get_logger().debug(f"Sent: voltage={voltage}, current={current}")

    def _send_stop(self):
        """Send stop signal over CAN (8-byte all zeros)"""
        frame = jcan.Frame(self.can_tx_id, [0x00] * 8)
        self.bus.send(frame)

        self.is_sending = False
        self.send_timer.cancel()
        self.get_logger().info(f"Measurement complete, sent {self.data_index} readings")


def main(args=None):
    rclpy.init(args=args)
    node = PotentiostatMockNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
