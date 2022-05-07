__package__ = "autonomous"

# standard imports
import time
import rclpy
import numpy as np

# autonomous imports
import math_utils.transform as transform
from config.runtime_params import tracking_camera_extrinsics, pose_file
from config.ros_config import main_frame, tracking_pose_topic, rover_pose_topic
from localisation.ekf import Ekf


# ROS imports 
from rclpy.node import Node
from nav_msgs.msg import Odometry
from core.msg import RoverPose
from geometry_msgs.msg import Vector3Stamped

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Subscribes to the DGPS and IMU topics and publishes some combined, transformed versions of this information

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
CREATION:	06/05/2022
EDITED:		06/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""


class PoseConverter(Node):
    """
	ROS2 Node to listen to:
		- DGPS
		- IMU
	And publish 3 different transformed coordinate frames
    """
    def __init__(self, serial_number=t265_serial):

        super().__init__("ConverterNode")

		# subscribers
		self.imu = self.create_sbuscription(Vector3Stamped, "imu/euler", self.imu_callback, 10)
		self.dgps = self.create_sbuscription(WhatMessageType, "hmmm what topic to use", self.dgps_callback, 10)

		# publishers
        self.camera_pub = self.create_publisher(Odometry, tracking_pose_topic, 10)        
        self.rover_pose_pub = self.create_publisher(RoverPose, rover_pose_topic, 10)
		
		# same as above but slightly different format
        self.rover_pose_odom_pub = self.create_publisher(Odometry, "rover/odom", 10)

        self.ekf = Ekf()
	
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

    def transform_t265_to_nova(self, imu_msg, dgps_msg):
        """
        Transform the raw T265 data into a ROS Odom message, with the right handed coorddinate system 
        where 
        up = +z
        left = +y
        forward = +x
        """
        t265_msg = Odometry()

        t265_msg.header.stamp = self.get_clock().now().to_msg()
        t265_msg.header.frame_id = main_frame

        x = -data.translation.z
        y = -data.translation.x
        z = data.translation.y
        
        x -= tracking_camera_extrinsics[0]
        y -= tracking_camera_extrinsics[1]
        z -= tracking_camera_extrinsics[2]

        t265_msg.pose.pose.orientation.x = -data.rotation.z
        t265_msg.pose.pose.orientation.y = -data.rotation.x
        t265_msg.pose.pose.orientation.z = data.rotation.y
        t265_msg.pose.pose.orientation.w = data.rotation.w
        
        if self.initial_yaw != 0:
            pitch, roll, yaw = transform.quat_to_euler(t265_msg)
            qx, qy, qz, qw = transform.euler_to_quat([pitch, roll, yaw + self.initial_yaw])
            
            t265_msg.pose.pose.orientation.x = qx
            t265_msg.pose.pose.orientation.y = qy
            t265_msg.pose.pose.orientation.z = qz
            t265_msg.pose.pose.orientation.w = qw

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

        return t265_msg

    def publisher_timer(self):
		
		
	
		t265_msg = self.transform_t265_to_nova(data)
		self.camera_pub.publish(t265_msg)
		rover_msg = RoverPose()
			
		# get rover position as centre of wheel-base
		rover_position = transform.transform_points(t265_msg, np.array([tracking_camera_extrinsics]))[0]

		rover_odom_msg = self.transform_t265_to_nova(data)
		rover_odom_msg.pose.pose.position.x = rover_position[0]
		rover_odom_msg.pose.pose.position.y = rover_position[1]
		rover_odom_msg.pose.pose.position.z = rover_position[2]
		rover_msg.x = rover_position[0]
		rover_msg.y = rover_position[1]
		rover_msg.z = rover_position[2]
			
		# gets euler angles from tracking camera quaternion
		rover_msg.pitch, rover_msg.roll, rover_msg.yaw = transform.quat_to_euler(t265_msg)
		self.rover_pose_pub.publish(rover_msg)
		self.rover_pose_odom_pub.publish(rover_odom_msg)

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
		with open(pose_file, "w") as f:
			f.write(f"{rover_msg.x}\t{rover_msg.y}\t{rover_msg.z}\t{rover_msg.yaw}")
		

def main():
    rclpy.init()
    converter = PoseConverter()
	rclpy.spin(converter)
	rclpy.shutdown()

if __name__ == "__main__":
    main()
