import rclpy
from rclpy.node import Node
from sensor_msgs.msg import PointCloud2


class SubscriberNode(Node):
    def __init__(self):
        # init node with node name points
        super().__init__('points_sub_simple')
        self.subscriber_points = self.create_subscription(PointCloud2, '/D435/depth/color/points', self.points_callback, 10)

    def points_callback(self, msg):
        print(type(msg))
        print("inspecting")


def main():
    rclpy.init(args=None)
    subscriber = SubscriberNode()
    rclpy.spin(subscriber)
    subscriber.destroy_node()


if __name__ == '__main__':
    main()
