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
from launch.actions import OpaqueFunction, DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def launch_setup(context, *args, **kwargs):
    port_name = LaunchConfiguration('port')

    return [
        Node(
            package='dgnss',
            namespace='',
            executable='gps_rover.py',
            name='gps_rover',
            parameters=[{'port_name': port_name}],
        ),
        # Node(
        #     package='electronics',
        #     namespace='',
        #     executable='magnetometer.py',
        #     name='magnetometer',
        #     parameters=[gps_params],
        # ),
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