#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from rclpy.client import Client
from sensor_msgs.msg import NavSatFix
from nova_interfaces.srv import CartographerCommand


class TestWaypointNavigator(Node):
    def __init__(self):
        super().__init__('waypoint_navigator')

        self._type = self.declare_parameter(
            name='type', 
            value=1, 
        ).value

        # 📝 Create service client for cartographer command server
        self._cartographer_client = self.create_client(CartographerCommand, '/autonomous/cartographer_command')
        self.get_logger().info('⏳ Waiting for /autonomous/cartographer_command server...')
        if not self._cartographer_client.wait_for_service(timeout_sec=10.0):
            self.get_logger().error('❌ Service /autonomous/cartographer_command not available. Exiting.')
            return
        self.get_logger().info('✅ Service /autonomous/cartographer_command available!')

        # 📝 Call the cartographer command service
        self.call_cartographer_async()


    def call_cartographer_async(self):
        cartographer_msg = CartographerCommand.Request()
        cartographer_msg.goals = self.create_goals()
        cartographer_msg.types = [0, 0, self._type]
        self.get_logger().info(f'🚀 Sending cartographer command to /autonomous/cartographer_command...')
        send_future = self._cartographer_client.call_async(cartographer_msg)
        send_future.add_done_callback(self.result_cartographer_callback)

    def result_cartographer_callback(self, future):
        result = future.result()

        if result.success:
            self.get_logger().info('✅ Cartographer command sent successfully!')
        else:
            self.get_logger().error('❌ Failed to send cartographer command. Exiting.')

    def create_goals(self):
        '''Creates a list of goals for the cartographer command.'''
        goals = []

        goal = NavSatFix()
        goal.latitude = 38.3672524
        goal.longitude = -110.7150385
        goals.append(goal)

        goal = NavSatFix()
        goal.latitude = 38.3669546
        goal.longitude = -110.7151176
        goals.append(goal)


        goal = NavSatFix()
        goal.latitude = 38.3668352
        goal.longitude = -110.7146483
        goals.append(goal)

        self.get_logger().info('✅ Created goals:')
        for goal in goals:
            self.get_logger().info(f'📍 Goal: {goal.latitude}, {goal.longitude}')

        return goals

def main(args=None):
    '''Main function to start the ROS2 node.'''
    rclpy.init(args=args)
    node = TestWaypointNavigator()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()