import numpy as np
import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
import time
import open3d as o3d
import copy

class SubscriberNode(Node):
    def __init__(self):
        # init node with node name points
        super().__init__('tracking')
        self.subscriber_points = self.create_subscription(Odometry, '/T265/odom/sample', self.callback, 10)


    def callback(self, msg):

        mesh = o3d.geometry.TriangleMesh.create_coordinate_frame()

        #o3d.visualization.draw_geometries([mesh])

        x = msg.pose.pose.position.x
        y = msg.pose.pose.position.y
        z = msg.pose.pose.position.z
        xquar = msg.pose.pose.orientation.x
        yquar = msg.pose.pose.orientation.y
        zquar = msg.pose.pose.orientation.z
        wquar = msg.pose.pose.orientation.w

        quar = np.array([xquar,yquar,zquar,wquar])

        mat = np.zeros((4,4))
        mat[:3,:3] = o3d.geometry.get_rotation_matrix_from_quaternion(quar)
        mat[3,3] = 1

        # print(mat)
        mesh_translate = copy.deepcopy(mesh).translate((x, y, z))
        x = "x: " + str(round(x, 4)).rjust(6)
        y = "y: " + str(round(y, 4)).rjust(6)
        z = "z: " + str(round(z, 4)).rjust(6)

        #print(f'Center: {mesh.get_center()}')
        print(x + " | " + y + " | " + z)
        #print(f'current pos : {mesh_translate.get_center()}')
        #o3d.visualization.draw_geometries([mesh])
        #o3d.visualization.draw_geometries([mesh_translate])
        time.sleep(.5)


def main():
    rclpy.init(args=None)
    subscriber = SubscriberNode()
    rclpy.spin(subscriber)
    subscriber.destroy_node()


if __name__ == '__main__':
    main()
