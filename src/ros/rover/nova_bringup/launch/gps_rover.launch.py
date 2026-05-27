'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Takes RTCM error correction data from 
base (ublox) GPS and writes data to the rover 
(skytraq) GPS over USB.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - /gps_rover
TOPICS:
  - subscriber: /gps_base/rtcm  [UInt8MultiArray]
  - publisher: /gps_rover/fix   [NavSatFix]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_bringup
AUTHOR(S):  Victor Bartlinski
CREATION:   30/04/2025
EDITED:     30/04/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, IfElseSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from os.path import expanduser

def launch_setup(context, *args, **kwargs):
    params = LaunchConfiguration('params')
    drone = LaunchConfiguration('drone')
    publish_fix_custom = LaunchConfiguration('publish_fix_custom')
    gps_node_name = IfElseSubstitution(
        drone,
        'drone_gps_rover',
        'gps_rover'
    )

    return [
        Node(
            package='dgnss',
            namespace='',
            executable='gps_rover.py',
            name=gps_node_name,
            parameters=[params, {'publish_fix_custom': publish_fix_custom}],
        ),
    ]

def generate_launch_description():
    local = LaunchConfiguration("local")

    nova_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/nova_bringup']),
        FindPackageShare('nova_bringup')
    )

    declared_arguments = [
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use the local source directory instead of the nix store for param files.',
        ),
        DeclareLaunchArgument(
            name='params',
            default_value=PathJoinSubstitution([nova_bringup_dir, 'params', 'gps_rover.yaml']),
            description='Path to gps rover parameter file',
        ),
        DeclareLaunchArgument(
            name='drone',
            default_value='False',
            description='Use drone gps instead of rover gps'
        ),
        DeclareLaunchArgument(
            name='publish_fix_custom',
            default_value='True',
            description='Whether to publish the custom GPSData message on /gps_rover/fix_custom'
        ),
        DeclareLaunchArgument(
            name='log_level',
            default_value='info',
            description='Log level of launched nodes and launch files'
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )