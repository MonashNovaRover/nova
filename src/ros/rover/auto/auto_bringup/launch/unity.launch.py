'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to publish the urdf
    static transforms and associated joint states
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - robot_state_publisher
  - rover_state_publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	27/04/2023
EDITED:     26/09/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction, TimerAction, ExecuteProcess, RegisterEventHandler
from launch.substitutions import  PathJoinSubstitution, LaunchConfiguration
from launch.conditions import IfCondition, UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.event_handlers import OnProcessExit

def launch_setup(context, *args, **kwargs):

    return [
        Node(
            package='image_transport',
            executable='republish',
            parameters=[{
                'in_transport': 'compressed',
                'out_transport': 'raw',
            }],
            remappings=[
                ('in/compressed', '/oak/rgb/image_compressed'),
                ('out', '/oak/rgb/image_raw'),
            ],
        ),
        Node(
            package='image_transport',
            executable='republish',
            parameters=[{
                'in_transport': 'compressed',
                'out_transport': 'raw',
            }],
            remappings=[
                ('in/compressed', '/bootie/rgb/image_compressed'),
                ('out', '/bootie/rgb/image_raw'),
            ],
        ),
        Node(
            package='ros_tcp_endpoint',
            executable='default_server_endpoint',
        ),
    ]

def generate_launch_description():

    declared_arguments = [
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
