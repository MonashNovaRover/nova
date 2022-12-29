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
from launch_ros.actions import Node

# Generate the launch file with all inputs
def generate_launch_description():
    return LaunchDescription([
        Node(
            package='control',
            node_executable='drive_inputs',
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='control',
            node_executable='driver',
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='electronics',
            node_executable='wheel_publisher.py',
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='electronics',
            node_executable='gimbal_service.py',
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='electronics',
            node_executable='LED_transmitter.py',
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='imu',
            node_executable='imu_node',
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='electronics',
            node_executable='CMD_service.py',
            output='screen',
            emulate_tty=True
        ),

        # tf2 static transformations
        # Add a new node defining a transform between any two frames necessary (ie GPS, imu, etc)
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            # TODO: Work out true extrinsics of camera relative to base of rover
            arguments=['0', '0', '0', '0', '0', '0', 'world', 'map'],
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            # TODO: Work out true extrinsics of camera relative to base of rover
            arguments=['0.48', '0', '0.48', '0', '0', '0', 'base_link', 'd435_1'],
            output='screen',
            emulate_tty=True
        ),
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            # TODO: Work out true extrinsics of camera relative to base of rover
            arguments= ['0.48', '0', '0.46', '0', '0', '0', 'base_link', 't265'],
            output='screen',
            emulate_tty=True
        )
    ])
