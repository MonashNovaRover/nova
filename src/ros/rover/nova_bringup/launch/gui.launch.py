'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the base station to run the
    gui. This does not do prerequisites of running
    the gui, like installing yarn packages, or
    linking!.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CREATION:   12/07/2025
EDITED:     12/07/2025
EDITED BY:  Felicity Matthews, Bailey Chessum
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.substitutions import PathJoinSubstitution
from launch.actions import IncludeLaunchDescription, OpaqueFunction, ExecuteProcess
from launch_ros.substitutions import FindPackageShare
from launch_xml.launch_description_sources import XMLLaunchDescriptionSource

def launch_setup(context, *args, **kwargs):
    rosbridge_dir = FindPackageShare('rosbridge_server')

    return [
        # Launch Rosbridge
        IncludeLaunchDescription(
            launch_description_source=XMLLaunchDescriptionSource(PathJoinSubstitution([rosbridge_dir, 'launch', 'rosbridge_websocket_launch.xml'])),
        ),
        ExecuteProcess(
            cmd=['yarn', 'dev', '--port', '5173'],
            # shell=True,
            cwd='~/nova/src/ros/nova-gui/nova-gui',
            output="screen"
        ),
        ExecuteProcess(
            cmd=['xdg-open', 'http://localhost:5173'],
            output="screen"
        ),
    ]

def generate_launch_description():
    declared_arguments = [
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
