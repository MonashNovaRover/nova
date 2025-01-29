import rclpy
from rclpy.node import Node
from sensor_msgs.msg import BatteryState
from battery_status import BatteryStatusNode
import unittest
import time

class TestBatteryStatusNode(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        rclpy.init()
        cls.node = BatteryStatusNode()
        cls.executor = rclpy.executors.SingleThreadedExecutor()
        cls.executor.add_node(cls.node)
        cls.spin_thread = threading.Thread(target=cls.executor.spin, daemon=True)
        cls.spin_thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.executor.shutdown()
        cls.node.destroy_node()
        rclpy.shutdown()

    def setUp(self):
        self.received_messages = []
        self.subscription = self.node.create_subscription(
            BatteryState,
            'battery_status',
            self.battery_status_callback,
            10
        )

    def tearDown(self):
        self.node.destroy_subscription(self.subscription)

    def battery_status_callback(self, msg):
        self.received_messages.append(msg)

    def test_battery_status_publishing(self):
        # Wait for messages to be published
        time.sleep(5)

        # Check if messages were received
        self.assertGreater(len(self.received_messages), 0, "No messages received")

        # Check the content of the received messages
        for msg in self.received_messages:
            self.assertIsInstance(msg, BatteryState)
            self.assertEqual(len(msg.cell_voltage), 8)
            self.assertGreaterEqual(msg.voltage, 0)
            self.assertGreaterEqual(msg.current, 0)

if __name__ == '__main__':
    unittest.main()