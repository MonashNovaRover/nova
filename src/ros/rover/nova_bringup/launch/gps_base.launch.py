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
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    gps_params = LaunchConfiguration('gps_params')

    return [
        Node(
            package='electronics',
            namespace='',
            executable='gps_base.py',
            name='gps_base',
            parameters=[gps_params],
        ),
    ]

def generate_launch_description():
    nova_bringup_dir = FindPackageShare('nova_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='gps_params', 
            default_value=PathJoinSubstitution([nova_bringup_dir, 'params', 'gps.yaml']), 
            description='Absolute filepath to GPS parameters',
        ),     
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )