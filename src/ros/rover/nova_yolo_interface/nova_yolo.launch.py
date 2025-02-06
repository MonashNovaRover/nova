'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code to run cube detection on sim
    Launches yolo.launch.py with specified args
Listens to 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - yolo_node
  - debug_node
  - tracking_node
  - detect_3d_node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	06/02/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

import os
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    model_dir = PathJoinSubstitution([FindPackageShare('yolo_bringup'), 'models', 'model.pt'])
    return LaunchDescription(
        [
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    os.path.join(
                        get_package_share_directory("yolo_bringup"),
                        "launch",
                        "yolo.launch.py",
                    )
                ),
                launch_arguments={
                    "model": LaunchConfiguration("model", default=model_dir),
                    "tracker": LaunchConfiguration("tracker", default="bytetrack.yaml"),
                    "device": LaunchConfiguration("device", default="cpu"),
                    "enable": LaunchConfiguration("enable", default="True"),
                    "threshold": LaunchConfiguration("threshold", default="0.5"),
                    "input_image_topic": LaunchConfiguration(
                        "input_image_topic", default="/oak/rgb/image_rect"
                    ),
                    "input_depth_topic": LaunchConfiguration(
                        "input_depth_topic", default="/oak/depth"
                    ),
                    "input_depth_info_topic": LaunchConfiguration(
                        "input_depth_info_topic", default="/oak/camera_info"
                    ),
                    "image_reliability": LaunchConfiguration(
                        "image_reliability", default="1"
                    ),
                    "namespace": LaunchConfiguration("namespace", default="yolo"),
                    "use_3d": LaunchConfiguration("use_3d", default="True"),
                    "use_tracking": LaunchConfiguration("use_tracking", default="True"),
                    "Imgsz_height": LaunchConfiguration("Imgsz_height", default="640"),
                    "Imgsz_width": LaunchConfiguration("Imgsz_width", default="640"),
                }.items(),
            )
        ]
    )
