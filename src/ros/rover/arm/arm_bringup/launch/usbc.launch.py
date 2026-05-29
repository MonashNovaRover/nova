'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Launch the USB-C localiser with aruco detection.
Starts the aruco tracker and usbc_localiser nodes.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	arm_bringup
CREATION:	28/05/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution

from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def launch_setup(context, *args, **kwargs):
    params = LaunchConfiguration('usbc_params')
    aruco_params = LaunchConfiguration('aruco_params')

    return [
        Node(
            package='auto_typing',
            executable='camera_info_publisher.py',
            parameters=[params],
        ),
        Node(
            package='aruco_opencv',
            executable='aruco_tracker_autostart',
            arguments=['--ros-args', '--params-file', aruco_params],
        ),
        Node(
            package='auto_typing',
            executable='usbc_localiser.py',
            parameters=[params],
        ),
    ]


def generate_launch_description():
    arm_bringup_dir = FindPackageShare('arm_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='usbc_params',
            default_value=PathJoinSubstitution([arm_bringup_dir, 'params', 'typing.yaml']),
            description='Path to USB-C localiser params file',
        ),
        DeclareLaunchArgument(
            name='aruco_params',
            default_value=PathJoinSubstitution([arm_bringup_dir, 'params', 'usbc_aruco_tracker.yaml']),
            description='Path to ArUco tracker params file',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
