import unittest
from unittest.mock import MagicMock
from battery_state import BatteryStateNode

class TestBatteryStateNode(unittest.TestCase):
    def setUp(self):
        self.node = BatteryStateNode()
        self.node.get_logger = MagicMock()

    def test_can_message_received_voltage_frame_1(self):
        message = MagicMock()
        message.id = self.node.VOLTAGE_RECV_FRAME_ID_1
        message.data = bytes([0x0A, 0x00, 0x14, 0x00, 0x1E, 0x00, 0x28, 0x00])  # Example data

        self.node.can_message_received(message)

        expected_voltages = [2.56, 5.12, 7.68, 10.24]
        self.assertEqual(self.node.battery_state.cell_voltage[0:4], expected_voltages)

    def test_can_message_received_voltage_frame_2(self):
        message = MagicMock()
        message.id = self.node.VOLTAGE_RECV_FRAME_ID_2
        message.data = bytes([0x32, 0x00, 0x3C, 0x00, 0x46, 0x00, 0x50, 0x00])  # Example data

        self.node.can_message_received(message)

        expected_voltages = [12.8, 15.36, 17.92, 20.48]
        self.assertEqual(self.node.battery_state.cell_voltage[4:8], expected_voltages)

    def test_can_message_received_current_voltage_frame(self):
        message = MagicMock()
        message.id = self.node.CURRENT_VOLTAGE_RECV_FRAME_ID
        message.data = bytes([0x00, 0x64, 0x00, 0xC8])  # Example data

        self.node.can_message_received(message)

        expected_current = 0.1
        expected_voltage = 0.2
        self.assertEqual(self.node.battery_state.current, expected_current)
        self.assertEqual(self.node.battery_state.voltage, expected_voltage)

if __name__ == '__main__':
    unittest.main()