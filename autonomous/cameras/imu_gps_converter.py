__package__ = "autonomous"

# standard imports
import time
import rclpy
import numpy as np

# autonomous imports
import math_utils.transform as transform
import sys
from config.runtime_params import pose_file, dgps_extrinsics
from config.ros_config import main_frame, tracking_pose_topic, rover_pose_topic


# ROS imports 
from rclpy.node import Node
from nav_msgs.msg import Odometry
from core.msg import RoverPose
from geometry_msgs.msg import Vector3Stamped
from ublox_ubx_msgs.msg import UBXNavPVT
from sensor_msgs.msg import Imu
from core.msg import RoverPoseGPS
from rclpy.qos import qos_profile_sensor_data as qos

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Subscribes to the DGPS and Imu topics and publishes some combined, transformed versions of this information

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: ConverterNode
TOPICS:
  - /imu/euler   [Subscribed]
  - /            [Subscribed]
  - /imu/euler   [Publisher]
  - /            [Published]
  - /            [Published]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:  autonomous    
AUTHOR(S): Liam Whitle
CREATION:    06/05/2022
EDITED:        06/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""


class PoseConverter(Node):
    """
    ROS2 Node to listen to:
        - DGPS
        - Imu
    And publish 3 different transformed coordinate frames
    """
    def __init__(self):

        super().__init__("ConverterNode")

        # subscribers
        self.imu = self.create_subscription(Imu, "/imu/data", self.imu_callback, 10)
        self.dgps = self.create_subscription(UBXNavPVT, "/gps_rover/ubx_nav_pvt", self.dgps_callback, qos)

        # publishers
        self.camera_pub = self.create_publisher(Odometry, tracking_pose_topic, 10)        
        self.rover_pose_pub = self.create_publisher(RoverPose, rover_pose_topic, 10)
        self.rover_pose_odom_pub = self.create_publisher(Odometry, "rover/odom", 10) 
        self.gui_pose_pub = self.create_publisher(RoverPoseGPS, "/electronics/rover_pose_gps", 10)
        self.initial_yaw = 0
        self.initial_position= np.array([0, 0, 0])
        self.initial_lat = 0
        self.initial_lon = 0

        self.dgps_msg = None
        self.imu_msg = None

        self.create_timer(0.2, self.publisher_timer)
        
        # same as above but slightly different format
        self.rover_pose_odom_pub = self.create_publisher(Odometry, "rover/odom", 10)
    
    def imu_callback(self, msg):
        """
        updates msg
        """
        self.imu_msg = msg
    
    def dgps_callback(self, msg):
        """
        updates msg
        """
        self.dgps_msg = msg
        if self.initial_lat == 0:
            self.initial_lat = self.dgps_msg.lat
            self.initial_lon = self.dgps_msg.lon

    def gps_to_xyz(self, dgps_msg):
        EARTH_RADIUS_M = 6371000 
        z = dgps_msg.hmsl / 1000  # mm to m
        if self.initial_position[2] == 0:
            self.initial_position[2] = z

        z -= self.initial_position[2]
        # x and y relative to initial position
        x = (dgps_msg.lat - self.initial_lat) * 1e-7 * np.pi/180 * EARTH_RADIUS_M
        y = -(dgps_msg.lon - self.initial_lon) * 1e-7 * np.pi/180 * EARTH_RADIUS_M

        return x, y, 0

    def transform_dgps_imu_to_nova(self):
        t265_msg = Odometry()
        gui_msg = RoverPoseGPS()

        gui_msg.valid = True

        gui_msg.latitude = self.dgps_msg.lat * 1e-7
        gui_msg.longitude = self.dgps_msg.lon * 1e-7

        t265_msg.header.stamp = self.get_clock().now().to_msg()
        t265_msg.header.frame_id = main_frame

        x, y, z = self.gps_to_xyz(self.dgps_msg)
        

        x -= dgps_extrinsics[0]
        y -= dgps_extrinsics[1]
        z -= dgps_extrinsics[2]

        t265_msg.pose.pose.orientation = self.imu_msg.orientation
        
        pitch, roll, yaw = transform.quat_to_euler(t265_msg)
        yaw = -yaw
        qx, qy, qz, qw = transform.euler_to_quat([pitch, roll, yaw + self.initial_yaw])
        
        t265_msg.pose.pose.orientation.x = qx
        t265_msg.pose.pose.orientation.y = qy
        t265_msg.pose.pose.orientation.z = qz
        t265_msg.pose.pose.orientation.w = qw

        gui_msg.pitch, gui_msg.roll, gui_msg.yaw = pitch * 180/np.pi, roll * 180/np.pi, yaw * 180/np.pi

        # translating local x and y into our frame
        rotation = np.array([[np.cos(self.initial_yaw), -np.sin(self.initial_yaw)], [np.sin(self.initial_yaw), np.cos(self.initial_yaw)]])
        
        pose = np.matmul(rotation, np.array([x, y]).T).T
        x, y = pose[0], pose[1]
 
        t265_msg.pose.pose.position.x = x
        t265_msg.pose.pose.position.y = y
        t265_msg.pose.pose.position.z = z

        # add offset from extrinsics and our initial pose
        t265_msg.pose.pose.position.x += self.initial_position[0]
        t265_msg.pose.pose.position.y += self.initial_position[1]
        t265_msg.pose.pose.position.z += self.initial_position[2]

        return t265_msg, gui_msg

    def publisher_timer(self):
        if not self.dgps_msg or not self.imu_msg:
            return
        t265_msg, gui_msg = self.transform_dgps_imu_to_nova()
        self.camera_pub.publish(t265_msg)
        rover_msg = RoverPose()
            
        # get rover position as centre of wheel-base
        rover_position = transform.transform_points(t265_msg, np.array([dgps_extrinsics]))[0]

        rover_odom_msg = t265_msg
        rover_odom_msg.pose.pose.position.x = rover_position[0]
        rover_odom_msg.pose.pose.position.y = rover_position[1]
        rover_odom_msg.pose.pose.position.z = 0.
        rover_msg.x = rover_position[0]
        rover_msg.y = rover_position[1]
        rover_msg.z = rover_position[2]
            
        # gets euler angles from tracking camera quaternion
        rover_msg.pitch, rover_msg.roll, rover_msg.yaw = transform.quat_to_euler(t265_msg)

        self.rover_pose_pub.publish(rover_msg)
        self.rover_pose_odom_pub.publish(rover_odom_msg)
        self.gui_pose_pub.publish(gui_msg)

        self.print_rover_msg(rover_msg)        
    
    
    def print_rover_msg(self, rover_msg):
        # write to system
        sys.stdout.write("\r" + "x: " + str(round(rover_msg.x, 4)).ljust(7)
                         + " | y: " + str(round(rover_msg.y, 4)).ljust(7)
                         + " | z: " + str(round(rover_msg.z, 4)).ljust(7)
                         + " | pitch: " + str(round(rover_msg.pitch, 4)).ljust(7)
                         + " | roll: " + str(round(rover_msg.roll, 4)).ljust(7)
                         + " | yaw: " + str(round(rover_msg.yaw, 4)).ljust(7))
        sys.stdout.flush()

        # I guess we still do this?
        #jwith open(pose_file, "w") as f:
        #    f.write(f"{rover_msg.x}\t{rover_msg.y}\t{rover_msg.z}\t{rover_msg.yaw}")
        

def main():
    rclpy.init()
    converter = PoseConverter()
    rclpy.spin(converter)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
