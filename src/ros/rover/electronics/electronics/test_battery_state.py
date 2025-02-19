import unittest
from unittest.mock import MagicMock, patch
from battery_state import BatteryStateNode
from sensor_msgs.msg import BatteryState

class TestBatteryStateNode(unittest.TestCase):

    @patch('src.ros.rover.electronics.electronics.battery_state.ControllerNode')
    def setUp(self, MockControllerNode):
        self.node = BatteryStateNode()
        self.node.publisher_ = MagicMock()
        self.node.get_logger = MagicMock()

    def test_can_message_received_voltage_frame_1(self):
        message = MagicMock()
        message.id = self.node.VOLTAGE_RECV_FRAME_ID_1
        message.data = b'\x0c\x80\x0c\x80\x0c\x80\x0c\x80'  # Example data

        self.node.parse_cell_voltages = MagicMock(return_value=[3.2, 3.2, 3.2, 3.2])
        self.node.can_message_received(message)

        self.assertEqual(self.node.battery_state.cell_voltage[0:4], [3.2, 3.2, 3.2, 3.2])

    def test_can_message_received_voltage_frame_2(self):
        message = MagicMock()
        message.id = self.node.VOLTAGE_RECV_FRAME_ID_2
        message.data = b'\x0c\x80\x0c\x80\x0c\x80\x0c\x80'  # Example data

        self.node.parse_cell_voltages = MagicMock(return_value=[3.2, 3.2, 3.2, 3.2])
        self.node.can_message_received(message)

        self.assertEqual(self.node.battery_state.cell_voltage[4:8], [3.2, 3.2, 3.2, 3.2])

    def test_can_message_received_current_voltage_frame(self):
        message = MagicMock()
        message.id = self.node.CURRENT_VOLTAGE_RECV_FRAME_ID
        message.data = b'\x0c\x80\x0c\x80'  # Example data

        self.node.parse_total_voltage_and_current = MagicMock(return_value=(3.2, 12.8))
        self.node.can_message_received(message)

        self.assertEqual(self.node.battery_state.current, 3.2)
        self.assertEqual(self.node.battery_state.voltage, 12.8)

if __name__ == '__main__':
    unittest.main()