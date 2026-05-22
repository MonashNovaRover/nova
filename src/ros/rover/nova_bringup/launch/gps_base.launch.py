'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Execute this code on the base station to 
read RTCM3 error correction data from base (ublox) 
GPS and publish to rover (skytraq) GPS.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - /gps_base
TOPICS:
  - publisher: /gps_base/fix    [NavSatFix]
  - publisher: /gps_base/rtcm   [UInt8MultiArray]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_bringup
AUTHOR(S):  Victor Bartlinski
CREATION:   30/04/2025
EDITED:     30/04/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import OpaqueFunction, DeclareLaunchArgument
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration, IfElseSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from os.path import expanduser

def launch_setup(context, *args, **kwargs):
    local = LaunchConfiguration('local')
    
    nova_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/home/nova/nova/src/ros/rover/nova_bringup']),
        FindPackageShare('nova_bringup')
    )

    gps_params = PathJoinSubstitution([nova_bringup_dir, 'params', 'gps.yaml'])

    return [
        Node(
            package='dgnss',
            namespace='',
            executable='gps_base.py',
            name='gps_base',
            parameters=[gps_params],
        ),
    ]

def generate_launch_description():
    declared_arguments = [
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use the local teleop_drive_joy source directory instead of the nix store for param files.',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )