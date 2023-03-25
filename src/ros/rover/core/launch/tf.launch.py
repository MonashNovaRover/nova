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
            executable='static_transform_publisher',
            arguments=['0', '0', '0', '0', '0', '0', 'world', 'map'],
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            arguments=['0.46', '0', '0.45', '0', '0', '0', 'base_link', 'd435_1_forward'],
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            arguments= ['0.45', '0', '0.49', '0', '0', '0', 'base_link', 't265_forward'],
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            arguments=[ '0', '0','0', str(-np.pi / 2), '0', str(-np.pi / 2 - np.pi * 8 / 180), 'd435_1_forward', 'd435_1'],
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            # Although the t265 is mounted at a 10 degree angle, the internal gyroscope accounts for this
            arguments= ['0', '0', '0', '0.5', '-0.5', '-0.5', '0.5', 't265_forward', 't265_footprint'],
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            # Although the t265 is mounted at a 10 degree angle, the internal gyroscope accounts for this
            arguments= ['0', '0', '0', str(-np.pi / 2), '0', str(np.pi / 2 - np.pi * 8 / 180), 't265_forward', 't265'],
            output='screen',
            emulate_tty=True
        ),
    ])
