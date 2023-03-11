"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   rover control scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - control/drive/drive_inputs      [drive_cmd]
  - control/drive/driver            [driver]
  - tf2 static transforms
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	15/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include the required launch parameters
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


# Generate the launch file with all inputs
def generate_launch_description():
    return LaunchDescription([
        # tf2 static transforms
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource([
                FindPackageShare("core"), '/launch', '/tf.launch.py'
            ])
        ),
        # autonomous nodes
        Node(
            package="autonomous",
            node_executable="main.py",
            output="screen",
            emulate_tty=True
        ),
        Node(
            package="autonomous",
            node_executable="depth_camera.py",
            output="screen",
            emulate_tty=True
        ),
        Node(
            package="autonomous",
            node_executable="pose_converter_ARC.py",
            output="screen",
            emulate_tty=True
        ),
        Node(
            package="autonomous",
            node_executable="GRUC.py",
            output="screen",
            emulate_tty=True
        ),
    ])
