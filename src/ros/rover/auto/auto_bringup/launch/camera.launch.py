from launch import LaunchDescription
from pathlib import Path

from launch_ros.actions import Node

camera_param_path = Path("/home/nova/nova/src/ros/rover/auto/auto_bringup/camera_params.yaml")

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='usb_cam',
            executable='usb_cam_node_exe',
            name='mapcam',
            output='screen',
            parameters=[camera_param_path]
        )
    ])

