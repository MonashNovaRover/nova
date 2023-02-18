"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   rover control scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - tf2 static transforms
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	core
CREATION:	30/12/22
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include the required launch parameters
from launch import LaunchDescription
from launch_ros.actions import Node
import numpy as np

# Generate the launch file with all inputs
def generate_launch_description():
    return LaunchDescription([
        # tf2 static transformations
        # Add a new node defining a transform between any two frames necessary (ie GPS, imu, etc)
        Node(
            package='tf2_ros',
            node_executable='static_transform_publisher',
            # TODO: Work out true extrinsics of camera relative to base of rover
            arguments=['0', '0', '0', '0', '0', '0', 'world', 'map'],
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='tf2_ros',
            node_executable='static_transform_publisher',
            # TODO: Work out true extrinsics of camera relative to base of rover
            arguments=['0.48', '0', '0.48', '0', '0', '0', 'base_link', 'd435_1'],
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='tf2_ros',
            node_executable='static_transform_publisher',
            # TODO: Work out true extrinsics of camera relative to base of rover
            arguments= ['0.48', '0', '0.46', '0', '0', '0', 'base_link', 't265'],
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='autonomous',
            node_executable='tracking_camera.py',
            output='screen',
            emulate_tty=True
        ),
    ])
