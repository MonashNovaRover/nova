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
PACKAGE: 	core
CREATION:	27/04/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, AppendEnvironmentVariable, OpaqueFunction, SetEnvironmentVariable
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch.conditions import UnlessCondition, IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')
    model = LaunchConfiguration('model')
    namespace = LaunchConfiguration('namespace')
    robot_name = LaunchConfiguration('robot_name')
    controllers = LaunchConfiguration('controllers')
    arm = LaunchConfiguration('arm')

    return [
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'control.launch.py'])),
            launch_arguments={'controllers': controllers}.items(),
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'urdf.launch.py'])),
            launch_arguments={'model': model, 'gazebo': 'true', 'robot_name': robot_name, 'arm': arm}.items(),
        ),
    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [
        DeclareLaunchArgument(
            name='controllers',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'controllers.yaml']), 
            description='Absolute path to controllers params file',
        ),
        DeclareLaunchArgument(
            name='launch_robot_description',
            default_value='True',
            description='Should gazebo launch its own robot description, or is one already running?',
        ),
        DeclareLaunchArgument(
            name='model', 
            default_value=PathJoinSubstitution([rover_description_dir, 'rover7', 'urdf', 'rover.urdf.xacro']), 
            description='Absolute path to robot urdf file',
        ),
        DeclareLaunchArgument(
            name='namespace',
            default_value='',
            description='Top-level namespace',
        ),
        DeclareLaunchArgument(
            name='robot_name',
            default_value='Rover7',
            description='name of the robot',
        ),
        # DeclareLaunchArgument(
        #     name='world',
        #     default_value=PathJoinSubstitution([nova_gazebo_dir, 'worlds', 'auto.sdf']),
        #     description='Full path to world model file to load',
        # ),
        DeclareLaunchArgument(
            name='arm',
            default_value='true',
            description='whether to launch arm',
        )
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )