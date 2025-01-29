import rclpy
from rclpy.node import Node
from sensor_msgs.msg import BatteryState
import jcan

class BatteryStatusNode(Node):
    def __init__(self):
        super().__init__('battery_status_node')
        self.publisher_ = self.create_publisher(BatteryState, 'battery_status', 10)
        self.can_interface = jcan.CanInterface('can0')  # Replace 'can0' with your CAN interface
        self.can_interface.add_listener(self.can_message_received)
        self.battery_state = BatteryState()
        self.battery_state.cell_voltage = [0.0] * 8  # Initialize with 8 cells
        self.get_logger().info('Battery Status Node has been started.')

    def can_message_received(self, message):
        if message.id == 0x4B0:
            voltages = self.parse_cell_voltages(message.data)
            self.battery_state.cell_voltage[0:4] = voltages
        elif message.id == 0x4B1:
            voltages = self.parse_cell_voltages(message.data)
            self.battery_state.cell_voltage[4:8] = voltages
        elif message.id == 0x4B2:
            current, total_voltage = self.parse_total_voltage_and_current(message.data)
            self.battery_state.current = current
            self.battery_state.voltage = total_voltage
        self.publish_status()

    def parse_cell_voltages(self, data):
        voltages = []
        for i in range(0, len(data), 2):
            voltage = int.from_bytes(data[i:i+2], byteorder='big') / 1000.0  # Convert to volts
            voltages.append(voltage)
        return voltages

    def parse_total_voltage_and_current(self, data):
        current = int.from_bytes(data[0:2], byteorder='big') / 1000.0  # Convert to amps
        total_voltage = int.from_bytes(data[2:4], byteorder='big') / 1000.0  # Convert to volts
        return current, total_voltage

    def publish_status(self):
        self.publisher_.publish(self.battery_state)
        self.get_logger().info(f'Published battery status: {self.battery_state}')

def main(args=None):
    rclpy.init(args=args)
    node = BatteryStatusNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()