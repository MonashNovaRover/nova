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
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def launch_setup(context, *args, **kwargs):
    port_name = LaunchConfiguration('port')

    return [
        Node(
            package='dgnss',
            namespace='',
            executable='gps_base.py',
            name='gps_base',
            parameters=[{'port_name': port_name}],
        ),
    ]

def generate_launch_description():
    declared_arguments = [
        DeclareLaunchArgument(
            name='port',
            default_value='/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0',
            description='Whether to use the local teleop_drive_joy source directory instead of the nix store for param files.',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )