#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, DurabilityPolicy
from sensor_msgs.msg import NavSatFix
from rosgraph_msgs.msg import Clock
from tf2_ros import Buffer, TransformListener
import numpy as np
import os
from datetime import datetime
import time
import threading
import sys
import termios
import tty

EARTH_RADIUS = 6378137.0  # meters


def gps_to_local(lat_ref, lon_ref, lat, lon):
    """Convert GPS (lat, lon) to local X, Y (m) using equirectangular approximation."""
    dlat = np.radians(lat - lat_ref)
    dlon = np.radians(lon - lon_ref)
    x = EARTH_RADIUS * dlon * np.cos(np.radians(lat_ref))
    y = EARTH_RADIUS * dlat
    return x, y


def keyboard_listener(callback):
    """Threaded keyboard listener that calls callback('s') when 's' is pressed."""
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    tty.setcbreak(fd)
    try:
        while True:
            key = sys.stdin.read(1)
            if key.lower() == 's':
                callback('s')
    except Exception:
        pass
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)


class PositionLogger(Node):
    def __init__(self):
        super().__init__('position_logger')

        # TF buffer/listener
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)

        best_effort_qos = QoSProfile(
            reliability=ReliabilityPolicy.BEST_EFFORT,
            durability=DurabilityPolicy.VOLATILE,
            depth=10
        )

        # Subscriptions
        self.create_subscription(NavSatFix, '/gps_rover/fix', self.gps_callback, best_effort_qos)
        self.create_subscription(Clock, '/clock', self.clock_callback, best_effort_qos)

        # Timer for TF polling
        self.timer = self.create_timer(0.1, self.timer_callback)  # 10 Hz

        # Data storage
        self.odom_data = []
        self.map_data = []
        self.gps_data = []

        # State
        self.gps_ref = None
        self.odom_established = False
        self.current_time = None
        self.logging_started = False

        # Throttle bookkeeping
        self._last_log_time = {}

        # Output directory
        self.output_dir = os.path.join(os.getcwd(), "position_logs")
        os.makedirs(self.output_dir, exist_ok=True)

        # Start keyboard listener thread
        self.key_thread = threading.Thread(target=keyboard_listener, args=(self.key_callback,), daemon=True)
        self.key_thread.start()

        self.get_logger().info("Position Logger Node started (using /clock timestamps).")
        self.get_logger().info("Press 's' anytime to save logged data manually.")

    # --- Utility ------------------------------------------------------------
    def log_throttle(self, level, key, period_sec, msg):
        """Throttle log messages by 'key' every 'period_sec' seconds."""
        now = time.time()
        last = self._last_log_time.get(key, 0)
        if now - last > period_sec:
            self._last_log_time[key] = now
            getattr(self.get_logger(), level)(msg)

    def key_callback(self, key):
        """Handle keyboard input (trigger save on 's')."""
        if key == 's':
            self.get_logger().info("Manual save triggered via keyboard.")
            self.save_data()

    # --- ROS Callbacks ------------------------------------------------------
    def clock_callback(self, msg: Clock):
        self.current_time = msg.clock.sec + msg.clock.nanosec * 1e-9

    def timer_callback(self):
        if self.current_time is None:
            self.log_throttle("warn", "no_clock", 5.0, "Waiting for /clock messages...")
            return

        timestamp = self.current_time

        # Wheel Odom Traj
        try:
            trans_odom = self.tf_buffer.lookup_transform('odom', 'base_link', rclpy.time.Time())
            pos = trans_odom.transform.translation
            self.odom_data.append([timestamp, pos.x, pos.y, pos.z])
            self.odom_established = True
        except Exception:
            pass

        # SLAM Traj
        try:
            trans_map = self.tf_buffer.lookup_transform('map', 'base_link', rclpy.time.Time())
            pos = trans_map.transform.translation
            self.map_data.append([timestamp, pos.x, pos.y, pos.z])
        except Exception:
            pass

        # Announce once when logging starts
        if not self.logging_started and (self.odom_data or self.map_data):
            self.logging_started = True
            self.get_logger().info("Logging started — position data is now being recorded.")

    def gps_callback(self, msg: NavSatFix):
        if self.current_time is None:
            self.log_throttle("warn", "no_clock_gps", 5.0, "No /clock time available for GPS logging.")
            return
        timestamp = self.current_time

        if not self.odom_established:
            self.log_throttle("warn", "wait_odom", 5.0,
                              "Waiting for odom->base_link before initializing GPS zero point...")
            return

        # Set reference once
        if self.gps_ref is None:
            self.gps_ref = (msg.latitude, msg.longitude)
            self.get_logger().info(f"GPS reference set to: {self.gps_ref[0]:.8f}, {self.gps_ref[1]:.8f}")
            self.get_logger().info("GPS logging started — transforming GPS coordinates to local map frame.")

        # Convert GPS → local
        x, y = gps_to_local(self.gps_ref[0], self.gps_ref[1], msg.latitude, msg.longitude)
        self.gps_data.append([timestamp, x, y, msg.altitude])

    # --- Shutdown -----------------------------------------------------------
    def save_data(self):
        """Write data to NPZ files."""
        timestamp_str = datetime.now().strftime("%Y%m%d_%H%M%S")
        if self.odom_data:
            np.savez(os.path.join(self.output_dir, f"odom_{timestamp_str}.npz"),
                     data=np.array(self.odom_data))
        if self.map_data:
            np.savez(os.path.join(self.output_dir, f"map_{timestamp_str}.npz"),
                     data=np.array(self.map_data))
        if self.gps_data:
            np.savez(os.path.join(self.output_dir, f"gps_{timestamp_str}.npz"),
                     data=np.array(self.gps_data))
        self.get_logger().info(f"Saved .npz logs to {self.output_dir}")

    def destroy_node(self):
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = PositionLogger()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info("Shutting down gracefully...")
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
