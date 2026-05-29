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
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, IfElseSubstitution

from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

from os.path import expanduser

usbc_dir = expanduser('~') + '/nova/src/ros/rover/arm/arm_bringup'


def launch_setup(context, *args, **kwargs):
    localiser_params = LaunchConfiguration('localiser_params')
    camera_info_params = LaunchConfiguration('camera_info_params')
    aruco_params = LaunchConfiguration('aruco_params')

    return [
        Node(
            package='auto_typing',
            executable='camera_info_publisher.py',
            parameters=[camera_info_params],
        ),
        Node(
            package='aruco_opencv',
            executable='aruco_tracker_autostart',
            arguments=['--ros-args', '--params-file', aruco_params],
        ),
        Node(
            package='auto_typing',
            executable='usbc_localiser.py',
            parameters=[localiser_params],
        ),
    ]


def generate_launch_description():
    local = LaunchConfiguration('local')

    arm_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([usbc_dir]), FindPackageShare('arm_bringup')
    )

    declared_arguments = [
        DeclareLaunchArgument(
            name='local',
            default_value='True',
            description='Use local source directories instead of the nix store.',
        ),
        DeclareLaunchArgument(
            name='localiser_params',
            default_value=PathJoinSubstitution([arm_bringup_dir, 'params', 'localisers.yaml']),
            description='Path to localiser params file',
        ),
        DeclareLaunchArgument(
            name='camera_info_params',
            default_value=PathJoinSubstitution([arm_bringup_dir, 'params', 'camera_info', 'periscope.yaml']),
            description='Path to camera info params file',
        ),
        DeclareLaunchArgument(
            name='aruco_params',
            default_value=PathJoinSubstitution([arm_bringup_dir, 'params', 'aruco', 'usbc_tracker.yaml']),
            description='Path to ArUco tracker params file',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
