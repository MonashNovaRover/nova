'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code to run cube detection on sim
    Launches yolo.launch.py with specified args
A modified yolo launch file from yolo_ros package
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - yolo_node
  - debug_node
  - tracking_node
  - detect_3d_node
TOPICS:
  INPUTS:
    - /oak/rgb/image_rect
    - /oak/depth
    - /oak/camera_info
  OUTPUTS:
    - /yolo (many topics under this)
    - /yolo/detections_3d (important! used
                            for cube localisation)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
AUTHOR:     Anthony Lew
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
    model_dir = PathJoinSubstitution([FindPackageShare('auto_bringup'), 'resources', 'ARC_2025_sim', 'model.pt'])
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
                ### For more arguments go see yolo.launch.py in the yolo_ros repo: https://github.com/mgonzs13/yolo_ros
                launch_arguments={
                    "model": LaunchConfiguration("model", default=model_dir),
                    "tracker": LaunchConfiguration("tracker", default="bytetrack.yaml"),# could optimise our tracker params? currently using default ultralytic params
                    "device": LaunchConfiguration("device", default="cpu"),             # get CUDA working later?
                    "enable": LaunchConfiguration("enable", default="True"),            # check if the original yolo_node is actually needed or if we just need 3D and/or tracking
                    "threshold": LaunchConfiguration("threshold", default="0.5"),       # not sure what this does
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
                    "namespace": LaunchConfiguration("namespace", default="yolo"),      # sets the ROS topic output e.g /yolo/
                    "use_3d": LaunchConfiguration("use_3d", default="True"),            # enables 3D node
                    "use_tracking": LaunchConfiguration("use_tracking", default="True"),# enables tracking node
                    "Imgsz_height": LaunchConfiguration("Imgsz_height", default="640"), # must be same as model's trained image width and height
                    "Imgsz_width": LaunchConfiguration("Imgsz_width", default="640"),
                }.items(),
            )
        ]
    )
