#!/usr/bin/env python3

import rclpy
from python_control.ControllerNode import ControllerNode
from sensor_msgs.msg import BatteryState

class BatteryStateNode(ControllerNode):

    CAN_BUS = "can0"

    CURRENT_VOLTAGE_RECV_FRAME_ID = 0x4B2

    MAX_VOLTAGE = 33600 / 1000
    MIN_VOLTAGE = 26720 / 1000

    def __init__(self):
        super(BatteryStateNode, self).__init__(name="BatteryStateNode", can_bus=self.CAN_BUS)
        self.publisher = self.create_publisher(BatteryState, 'battery_state', 10)
        self.start_can()
        self.bus.add_callback(self.CURRENT_VOLTAGE_RECV_FRAME_ID, self.read_data_callback)

    def read_data_callback(self, frame):
        if frame.id != self.CURRENT_VOLTAGE_RECV_FRAME_ID:
            self.get_logger().info(f"Received unknown frame {frame}")
            return 
        
        self.get_logger().info(f"Received {hex(frame.id)} {frame.data}")

        current, voltage = self.parse_current_and_voltage(frame.data)
        self.publish_msg(current, voltage)
        
    def parse_current_and_voltage(self, data):
        current = int.from_bytes(data[0:2], 'big') / 1000.0
        voltage = int.from_bytes(data[2:4], 'big') / 1000.0
        return current, voltage

    def publish_msg(self, current, voltage):
        msg = BatteryState()
        msg.current, msg.voltage = current, voltage
        msg.percentage = max(0.0, min(100.0, 
            ((voltage - self.MIN_VOLTAGE) / (self.MAX_VOLTAGE - self.MIN_VOLTAGE)) * 100
        ))
        self.get_logger().info(f"Publishing {msg}")
        self.publisher.publish(msg)

def main(args=None):
    rclpy.init(args=args)
    node = BatteryStateNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()