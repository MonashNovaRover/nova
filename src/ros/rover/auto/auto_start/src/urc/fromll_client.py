#!/usr/bin/env python3
from rclpy.node import Node
from robot_localization.srv import FromLL
from geometry_msgs.msg import PoseStamped
from sensor_msgs.msg import NavSatFix

class FromLLClient():
    def __init__(self, node):
        # Set parameters
        self.node=node
        self.calling=False
        self.ll_iter=None
        self.ll=None
        self.poses=[]
        self.total_poses=None
        self.started=False

        # Create service client for robot_localization /fromLL
        self.fromll_client = self.node.create_client(FromLL, '/fromLL')
        self.node.get_logger().info('Waiting for /fromLL server...')
        self.started = self.fromll_client.wait_for_service(timeout_sec=10.0)
        if not self.started:
            self.node.get_logger().error('Failed to find service /fromLL! Exiting.')
            return
        self.node.get_logger().info('Successfully found service /fromLL.')

        # TODO
        # Add subscriber to /gps_rover/fix
        # Add client to /datum
        # Once fix received, request set /datum with the fix

    def call(self):
        '''Converts GNSS goal to a geometry_msgs/msg/Point using the robot_localization FromLL service.'''
        self.calling = True
        fromll_req = FromLL.Request()
        fromll_req.ll_point.latitude = self.ll.latitude
        fromll_req.ll_point.longitude = self.ll.longitude
        self.node.get_logger().info(f'Sending GNSS goal {self.ll.latitude}, {self.ll.longitude} to /fromLL...')
        future = self.fromll_client.call_async(fromll_req)
        future.add_done_callback(self.result)
    
    def result(self, future):
        result = future.result()
        pose = self.point_to_pose(result.map_point)
        self.poses.append(pose)
        self.calling = False

    def point_to_pose(self, point):
        '''Creates a waypoint from a geometry_msgs/msg/Point.'''
        pose = PoseStamped()
        pose.header.frame_id = 'map'
        pose.header.stamp = self.node.get_clock().now().to_msg()
        pose.pose.position.x = point.x
        pose.pose.position.y = point.y
        pose.pose.position.z = 0.0
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