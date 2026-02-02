'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

DESCRIPTION: Executes software for URC Auto Stack.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATED:	29/05/2025
EDITED:		29/05/2025
AUTHOR(S):  Victor Bartlinski
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.substitutions import  PathJoinSubstitution, LaunchConfiguration
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')

    rl_params = LaunchConfiguration('rl_params')
    nav2_params = LaunchConfiguration('nav2_params')
    use_composition = LaunchConfiguration('use_composition')
    yolo_params = LaunchConfiguration('yolo_params')

    return [
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'localization.launch.py'])),
            launch_arguments={
                'rl_params': rl_params,
            }.items()
        ),
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'navigation.launch.py'])),
            launch_arguments={
                'nav2_params': nav2_params,
                'use_composition': use_composition,
            }.items()
        ),
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'yolo.launch.py'])),
            launch_arguments={
                'yolo_params': yolo_params,
            }.items()
        ),
    ]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='rl_params',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','rl_urc.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='nav2_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2_urc.yaml']),
            description='Full path to the ROS2 parameters file to use for all launched Nav2 nodes',
        ),
        DeclareLaunchArgument(
            name='use_composition',
            default_value='False',
            description='Use composed bringup if True',
        ),
        DeclareLaunchArgument(
            name='yolo_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'yolo.yaml']),
            description='Full path to the ROS2 parameters file to use for all launched nodes',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
