#!/usr/bin/env python
# -*- coding: utf-8 -*-
## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2019 Intel Corporation. All Rights Reserved.

"""
This example shows how to fuse wheel odometry measurements (in the form of 3D translational velocity measurements) on the T265 tracking camera to use them together with the (internal) visual and intertial measurements.
This functionality makes use of two API calls:
1. Configuring the wheel odometry by providing a json calibration file (in the format of the accompanying calibration file)
Please refer to the description of the calibration file format here: https://github.com/IntelRealSense/librealsense/blob/master/doc/t265.md#wheel-odometry-calibration-file-format.
2. Sending wheel odometry measurements (for every measurement) to the camera
Expected output:
For a static camera, the pose output is expected to move in the direction of the (artificial) wheel odometry measurements (taking into account the extrinsics in the calibration file).
The measurements are given a high weight/confidence, i.e. low measurement noise covariance, in the calibration file to make the effect visible.
If the camera is partially occluded the effect will be even more visible (also for a smaller wheel odometry confidence / higher measurement noise covariance) because of the lack of visual feedback. Please note that if the camera is *fully* occluded the pose estimation will switch to 3DOF, estimate only orientation, and prevent any changes in the position.
"""

import pyrealsense2 as rs
import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry

config_file = "calibration_odometry.json"

class WheelOdomFuse(Node):
    def __init__(self):
        super().__init__('wheel_odom_node')

        #self.wheel_subscriber = self.create_subscription(RoverPose, "autonomous/pose", self.update_pose_with_wheeldata, 10)
        self.v = rs.vector() # wheel velocity input
        self.pose_publisher = self.create_publisher(Odometry, '/T265/odom/sample', 10)
        #self.timer = self.create_timer(0.5, self.callback_func)

        # load wheel odometry config before pipe.start(...)
        self.pipe = rs.pipeline()  # Declare RealSense pipeline, encapsulating the actual device and sensors
        cfg = rs.config()  # Build config object
        cfg.enable_stream(rs.stream.pose)

        # get profile/device/ wheel odometry sensor
        profile = cfg.resolve(self.pipe)
        dev = profile.get_device()
        tm2 = dev.as_tm2()

        self.wheel_odometer = None

        if tm2:
            # tm2.first_wheel_odometer()?
            pose_sensor = tm2.first_pose_sensor()
            self.wheel_odometer = pose_sensor.as_wheel_odometer()

            # load/configure wheel odometer
            self.wheel_odometer.load_wheel_odometery_config(self.toUint8(config_file))

            self.pipe.start(cfg)

    def toUint8(self, filename):
        # calibration to list of uint8
        f = open(filename)
        chars = []
        for line in f:
            for c in line:
                chars.append(ord(c))  # char to uint8
        return chars

    def update_pose_with_wheeldata(self, msg):
        self.v.x = msg.x # m/s
        self.v.y = msg.y
        self.v.z = msg.z

        # provide wheel odometry as vecocity measurement
        wo_sensor_id = 0  # indexed from 0, match to order in calibration file
        frame_num = 0  # not used
        self.wheel_odometer.send_wheel_odometry(wo_sensor_id, frame_num, self.v)

    def send_pose(self):
        self.pipe.start()
        try:
            while True:
                frames = self.pipe.wait_for_frames() # Wait for the next set of frames from the camera
                pose = frames.get_pose_frame() # Fetch pose frame
                if pose:
                    data = pose.get_pose_data()
                    print("Frame #{}".format(pose.frame_number))
                    print("Position: {}".format(data.translation))

                    # publish pose!
                    # need to inspect data object, might be of type Odometry
                    tracking_coords = Odometry()
                    tracking_coords.pose.pose.position.x = data.translation[0] # must be floats
                    tracking_coords.pose.pose.position.y = data.translation[1]
                    tracking_coords.pose.pose.position.z = data.translation[2]
                    self.pose_publisher.publish(tracking_coords)
        finally:
            self.pipe.stop()

def main():
    rclpy.init()
    wheelOdo = WheelOdomFuse()
    rclpy.spin(wheelOdo)
    wheelOdo.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()

