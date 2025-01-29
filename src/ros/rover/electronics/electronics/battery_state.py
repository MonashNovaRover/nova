import rclpy
from python_control.ControllerNode import ControllerNode
from sensor_msgs.msg import BatteryState
from nova_interfaces.msg import CANFrame

class BatteryStateNode(ControllerNode):

    # CAN BUS NAME
    CAN_BUS = "can0"

    # RECEIVING CARD IDS
    VOLTAGE_RECV_FRAME_ID_1 = 0x4B0
    VOLTAGE_RECV_FRAME_ID_2 = 0x4B1
    CURRENT_VOLTAGE_RECV_FRAME_ID = 0x4B2

    def __init__(self):
        super(BatteryStateNode, self).__init__(name="BatteryStateNode", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        # Add Publisher
        self.publisher_ = self.create_publisher(BatteryState, 'battery_state', 10)

        # Initialize BatteryState message
        self.battery_state = BatteryState()
        self.battery_state.cell_voltage = [0.0] * 8

        self.get_logger().info('Battery State Node has been started.')

    def can_message_received(self, message):
        if message.id == self.VOLTAGE_RECV_FRAME_ID_1:
            voltages = self.parse_cell_voltages(message.data)
            self.battery_state.cell_voltage[0:4] = voltages
        elif message.id == self.VOLTAGE_RECV_FRAME_ID_2:
            voltages = self.parse_cell_voltages(message.data)
            self.battery_state.cell_voltage[4:8] = voltages
        elif message.id == self.CURRENT_VOLTAGE_RECV_FRAME_ID:
            current, total_voltage = self.parse_total_voltage_and_current(message.data)
            self.battery_state.current = current
            self.battery_state.voltage = total_voltage
        self.publish_status()
        self.get_logger().info('CAN message received and processed.')

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

def main(args=None):
    rclpy.init(args=args)
    node = BatteryStateNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()