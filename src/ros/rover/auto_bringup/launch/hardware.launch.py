'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

DESCRIPTION: Executes hardware for URC Auto Stack.
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
    nova_bringup_dir = FindPackageShare('nova_bringup')

    gps_params = LaunchConfiguration('gps_params')
    controller_params = LaunchConfiguration('controller_params')
    front_params = LaunchConfiguration('front_params')
    back_params = LaunchConfiguration('back_params')

    return [
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([nova_bringup_dir, 'launch', 'gps_rover.launch.py'])),
            launch_arguments={
                'gps_params': gps_params,
            }.items(),
        ),
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'control.launch.py'])),
            launch_arguments={
                'controller_params': controller_params,
            }.items(),
        ),
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'camera.launch.py'])),
            launch_arguments={
                'front_params': front_params,
                'back_params': back_params,
            }.items(),
        ),
    ]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')
    nova_bringup_dir = FindPackageShare('nova_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='gps_params',
            default_value=PathJoinSubstitution([nova_bringup_dir, 'params', 'gps.yaml']),
            description='Absolute path to GPS params file',
        ),
        DeclareLaunchArgument(
            name='controller_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'controllers.yaml']),
            description='Absolute path to controller params file',
        ),
        DeclareLaunchArgument(
            name='front_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'oak.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='back_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'bootie.yaml']),
            description='',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
