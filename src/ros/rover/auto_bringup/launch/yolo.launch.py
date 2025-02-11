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
from launch_ros.actions import Node
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
                    "tracker": LaunchConfiguration("tracker", default="bytetrack.yaml"),        # could optimise our tracker params? currently using default ultralytic params
                    "device": LaunchConfiguration("device", default="cpu"),                     # get CUDA working later?
                    "enable": LaunchConfiguration("enable", default="True"),                    # 2d bounding box generator node
                    "threshold": LaunchConfiguration("threshold", default="0.5"),               # threshold to find a match e.g > 0.5 = a match
                    "target_frame": LaunchConfiguration("target_frame", default="camera_link"), # frame from which image originates (important for bb3d accuracy)
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
                    "depth_image_units_divisor": LaunchConfiguration(
                        "depth_image_units_divisor", default="1"
                    ),                                                                          # amount to divide to convert depth input to meters
                    "namespace": LaunchConfiguration("namespace", default="yolo"),              # sets the ROS topic output e.g /yolo/
                    "use_3d": LaunchConfiguration("use_3d", default="True"),                    # enables 3D node, needed for bb3d
                    "use_tracking": LaunchConfiguration("use_tracking", default="False"),       # enables tracking node, may not be needed?
                    "Imgsz_height": LaunchConfiguration("Imgsz_height", default="400"),         # must be same as model's trained image width and height
                    "Imgsz_width": LaunchConfiguration("Imgsz_width", default="600"),           # above comment may be wrong? as it works fine using depth cam's resolution
                    
                }.items(),
            ),
            Node(
                #condition=IfCondition(),
                package='nova_utils',
                executable='yolo_3d_to_marker.py',
                parameters=[{'namespace':'/yolo'}],
            ),
        ]
    )
