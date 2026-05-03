#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from robot_localization.srv import FromLL
from geometry_msgs.msg import PoseStamped, Point
from sensor_msgs.msg import NavSatFix
from tf2_ros import Buffer, TransformListener
from tf_transformations import quaternion_from_euler
import math

class FromLLClient():
    def __init__(self, node:Node):
        # Set parameters
        self.node=node
        self.calling=False
        self.ll_iter=None
        self.ll=None
        self.poses=[]
        self.total_poses=None
        self.prev_pose=PoseStamped()
        self.started=False

        # Create service client for robot_localization /fromLL
        self.fromll_client = self.node.create_client(FromLL, '/fromLL')
        self.node.get_logger().info('Waiting for /fromLL server...')
        self.started = self.fromll_client.wait_for_service(timeout_sec=10.0)
        if not self.started:
            self.node.get_logger().error('Failed to find service /fromLL! Exiting.')
            return
        self.node.get_logger().info('Successfully found service /fromLL.')

        # Set initial previous pose to be current rover pose
        # (i.e. transfrom map->base_link)
        try:
            tf_buffer = Buffer()
            tf_listener = TransformListener(tf_buffer, self.node) # DO NOT DELETE THIS (it updates TF buffer directly)
            while not tf_buffer.can_transform('map', 'base_link', rclpy.time.Time()):
                self.node.get_logger().info('Waiting for map and base_link to register...')
                rclpy.spin_once(self.node, timeout_sec=1)
            self.node.get_logger().info('Waiting for transform map->base_link...')
            transform = tf_buffer.lookup_transform('map', 'base_link', rclpy.time.Time(), rclpy.time.Duration(seconds=10.0))
            self.prev_pose.pose.position.x = transform.transform.translation.x
            self.prev_pose.pose.position.y = transform.transform.translation.y
            self.node.get_logger().info('Successfully found transform map->base_link.')
        except Exception as e:
            self.node.get_logger().error(f'Failed to get transform map->base_link! {e}')
            raise Exception

    def call(self):
        '''Converts GNSS goal to a geometry_msgs/msg/Point using the robot_localization FromLL service.'''
        self.calling = True
        fromll_req = FromLL.Request()
        fromll_req.ll_point.latitude = self.ll.latitude
        fromll_req.ll_point.longitude = self.ll.longitude
        self.node.get_logger().info(f'Sending GNSS goal ({self.ll.latitude}, {self.ll.longitude}) to /fromLL...')
        future = self.fromll_client.call_async(fromll_req)
        future.add_done_callback(self.result)
    
    def result(self, future):
        result = future.result()
        pose = self.point_to_pose(result.map_point)
        self.poses.append(pose)
        self.calling = False

    def point_to_pose(self, point):
        '''Creates a waypoint from a geometry_msgs/msg/Point.'''
        self.node.get_logger().info(f'Received pose goal ({point.x:.3f}, {point.y:.3f}) from /fromLL.')
        pose = PoseStamped()
        pose.header.frame_id = 'map'
        pose.header.stamp = self.node.get_clock().now().to_msg()
        pose.pose.position.x = point.x
        pose.pose.position.y = point.y
        pose.pose.position.z = 0.0

        # Set pose orientation to be same direction of incoming path 
        # (i.e. stop rover from turning at goal)
        dx = point.x - self.prev_pose.pose.position.x
        dy = point.y - self.prev_pose.pose.position.y
        self.prev_pose = pose
        yaw = math.atan2(dy, dx)
        q = quaternion_from_euler(0, 0, yaw)
        pose.pose.orientation.x = q[0]
        pose.pose.orientation.y = q[1]
        pose.pose.orientation.z = q[2]
        pose.pose.orientation.w = q[3]

        return pose

    def lls_to_poses(self, lls:list[NavSatFix]):
        self.ll_iter = iter(lls)
        self.total_poses = len(lls)
        self.poses = []

    def waiting(self):
        return self.calling

    def has_next(self):
        self.ll = next(self.ll_iter, None)
        return self.ll is not None
    
    def converted_goals(self):
        return len(self.poses) == self.total_poses