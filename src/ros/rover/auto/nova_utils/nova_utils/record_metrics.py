#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import NavSatFix
from rosgraph_msgs.msg import Clock
from tf2_ros import Buffer, TransformListener
import numpy as np
import os
from datetime import datetime

EARTH_RADIUS = 6378137.0


def gps_to_local(lat_ref, lon_ref, lat, lon):
    """Convert GPS (lat, lon) to local X, Y in meters using equirectangular approximation."""
    dlat = np.radians(lat - lat_ref)
    dlon = np.radians(lon - lon_ref)
    x = EARTH_RADIUS * dlon * np.cos(np.radians(lat_ref))
    y = EARTH_RADIUS * dlat
    return x, y


class PositionLogger(Node):
    def __init__(self):
        super().__init__('position_logger')

        # TF buffer/listener
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)

        # Subscriptions
        self.create_subscription(NavSatFix, '/gps/fix', self.gps_callback, 10)
        self.create_subscription(Clock, '/clock', self.clock_callback, 10)

        # Timer for transform polling
        self.timer = self.create_timer(0.1, self.timer_callback)  # 10 Hz

        # Data storage
        self.odom_data = []
        self.map_data = []
        self.gps_data = []

        # State variables
        self.gps_ref = None
        self.odom_established = False
        self.current_time = None

        # Output directory
        self.output_dir = os.path.join(os.getcwd(), "position_logs")
        os.makedirs(self.output_dir, exist_ok=True)

        self.get_logger().info("Position Logger Node started (using /clock timestamps).")

    def clock_callback(self, msg: Clock):
        self.current_time = msg.clock.sec + msg.clock.nanosec * 1e-9

    def timer_callback(self):
        if self.current_time is None:
            self.get_logger().warn_throttle(10, "Waiting for /clock messages...")
            return

        timestamp = self.current_time

        # odom -> base_link
        try:
            trans_odom = self.tf_buffer.lookup_transform('odom', 'base_link', rclpy.time.Time())
            pos = trans_odom.transform.translation
            self.odom_data.append([timestamp, pos.x, pos.y, pos.z])
            self.odom_established = True
        except Exception:
            pass

        # map -> base_link
        try:
            trans_map = self.tf_buffer.lookup_transform('map', 'base_link', rclpy.time.Time())
            pos = trans_map.transform.translation
            self.map_data.append([timestamp, pos.x, pos.y, pos.z])
        except Exception:
            pass

    def gps_callback(self, msg: NavSatFix):
        if self.current_time is None:
            self.get_logger().warn_throttle(10, "No /clock time available for GPS logging.")
            return
        timestamp = self.current_time

        if not self.odom_established:
            self.get_logger().warn_throttle(10, "Waiting for odom->base_link before initializing GPS zero point...")
            return

        # Set GPS reference point (zero position)
        if self.gps_ref is None:
            self.gps_ref = (msg.latitude, msg.longitude)
            self.get_logger().info(f"GPS reference set to: {self.gps_ref}")

        # Convert GPS coordinates to local (map-like) frame
        x, y = gps_to_local(self.gps_ref[0], self.gps_ref[1], msg.latitude, msg.longitude)
        self.gps_data.append([timestamp, x, y, msg.altitude])

    def save_data(self):
        """Save collected data to NPZ files."""
        timestamp_str = datetime.now().strftime("%Y%m%d_%H%M%S")
        if self.odom_data:
            np.savez(os.path.join(self.output_dir, f"odom_{timestamp_str}.npz"), data=np.array(self.odom_data))
        if self.map_data:
            np.savez(os.path.join(self.output_dir, f"map_{timestamp_str}.npz"), data=np.array(self.map_data))
        if self.gps_data:
            np.savez(os.path.join(self.output_dir, f"gps_{timestamp_str}.npz"), data=np.array(self.gps_data))
        self.get_logger().info(f"Saved .npz files in {self.output_dir}")

    def destroy_node(self):
        self.save_data()
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = PositionLogger()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info("Shutting down data recorder")
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
