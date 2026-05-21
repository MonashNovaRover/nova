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
  RX Ch1: 0x01A# (no data, just trigger)
  RX Ch2: 0x01B# (no data, just trigger)
  TX Ch1: 0x41A#CC CC CC CC VV VV VV VV
  TX Ch2: 0x41B#CC CC CC CC VV VV VV VV
      - CCCCCCCC: current in µA (int32, little-endian)
      - VVVVVVVV: voltage in mV (int32, little-endian)
  STOP: All zeros (8-byte) indicates last message
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PARAMETERS:
  can_bus: CAN interface name (default: "can1")
  can_trigger_id_ch1/ch2: CAN IDs for receiving triggers
  can_data_id_ch1/ch2: CAN IDs for sending data
  csv_file: Path to CSV file with current,voltage columns
  send_rate: Rate to send data in Hz (default: 10.0)
  can_spin_rate: CAN bus polling rate in Hz (default: 20.0)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CSV FORMAT (values in µA and mV, matching CAN protocol):
  current,voltage
  50000,1000  (= 50mA, 1.0V)
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

        self.can_trigger_id_ch1 = self.declare_parameter(
            "can_trigger_id_ch1", 0x01A,
            ParameterDescriptor(description="CAN ID for receiving channel 1 trigger")
        ).value

        self.can_trigger_id_ch2 = self.declare_parameter(
            "can_trigger_id_ch2", 0x01B,
            ParameterDescriptor(description="CAN ID for receiving channel 2 trigger")
        ).value

        self.can_data_id_ch1 = self.declare_parameter(
            "can_data_id_ch1", 0x41A,
            ParameterDescriptor(description="CAN ID for sending channel 1 data")
        ).value

        self.can_data_id_ch2 = self.declare_parameter(
            "can_data_id_ch2", 0x41B,
            ParameterDescriptor(description="CAN ID for sending channel 2 data")
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
        self.bus.add_callback(self.can_trigger_id_ch1, lambda f: self._on_trigger_receive(channel=0))
        self.bus.add_callback(self.can_trigger_id_ch2, lambda f: self._on_trigger_receive(channel=1))
        self.create_timer(1.0 / can_spin_rate, self.bus.spin)

        # Data send timer (starts disabled)
        self.send_period = 1.0 / send_rate
        self.send_timer = self.create_timer(self.send_period, self._send_data)
        self.send_timer.cancel()

        self.get_logger().info(
            f"Potentiostat mock initialized (Trigger: 0x{self.can_trigger_id_ch1:03X}/0x{self.can_trigger_id_ch2:03X}, "
            f"Data: 0x{self.can_data_id_ch1:03X}/0x{self.can_data_id_ch2:03X}, "
            f"rate: {send_rate}Hz, {len(self.data)} data points)"
        )

    def _load_csv(self, filepath: str):
        """Load current/voltage data from CSV file"""
        try:
            with open(filepath, 'r') as f:
                reader = csv.DictReader(f)
                for row in reader:
                    current = int(row['current'])
                    voltage = int(row['voltage'])
                    self.data.append((current, voltage))
            self.get_logger().info(f"Loaded {len(self.data)} data points from {filepath}")
        except FileNotFoundError:
            self.get_logger().error(f"CSV file not found: {filepath}")
        except KeyError as e:
            self.get_logger().error(f"CSV missing required column: {e}")
        except ValueError as e:
            self.get_logger().error(f"CSV parse error: {e}")

    def _on_trigger_receive(self, channel: int):
        """CAN callback - handle trigger command (channel determined by CAN ID)"""
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

        current, voltage = self.data[self.data_index]
        self._send_reading(current, voltage)
        self.data_index += 1

    def _send_reading(self, current: int, voltage: int):
        """Send current/voltage reading over CAN"""
        # Pack current as 4 bytes (little-endian, signed, in µA)
        current_bytes = current.to_bytes(4, 'little', signed=True)
        # Pack voltage as 4 bytes (little-endian, signed, in mV)
        voltage_bytes = voltage.to_bytes(4, 'little', signed=True)

        data = list(current_bytes) + list(voltage_bytes)
        data_id = self.can_data_id_ch1 if self.current_channel == 0 else self.can_data_id_ch2
        frame = jcan.Frame(data_id, data)
        self.bus.send(frame)

        self.get_logger().debug(f"Ch{self.current_channel} sent: current={current}µA, voltage={voltage}mV")

    def _send_stop(self):
        """Send stop signal over CAN (8-byte all zeros)"""
        data_id = self.can_data_id_ch1 if self.current_channel == 0 else self.can_data_id_ch2
        frame = jcan.Frame(data_id, [0x00] * 8)
        self.bus.send(frame)

        self.is_sending = False
        self.send_timer.cancel()
        self.get_logger().info(f"Ch{self.current_channel} measurement complete, sent {self.data_index} readings")


def main(args=None):
    rclpy.init(args=args)
    node = PotentiostatMockNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
