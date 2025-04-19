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
    nova_bringup_dir = FindPackageShare('nova_bringup')
    arm = LaunchConfiguration('arm')
    world = LaunchConfiguration('world')
    arm_params = LaunchConfiguration('arm_params')
    namespace = LaunchConfiguration('namespace')
    rviz_params = LaunchConfiguration('rviz_params')

    return [
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'gazebo.launch.py'])),
            launch_arguments={'namespace': namespace, 'world': world, 'arm': arm}.items(),
        ),
        Node(
            package='arm',
            executable='keyboard_localiser.py',
            name='keyboard_localiser',
            parameters=[arm_params],
            namespace=namespace,
        ),
        Node(
            package='rviz2',
            namespace='',
            executable='rviz2',
            name='rviz2',
            arguments=['-d', [PathJoinSubstitution([nova_bringup_dir, 'rviz', rviz_params])]]
        )
    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')
    rover_description_dir = FindPackageShare('rover_description')
    nova_bringup_dir = FindPackageShare('nova_bringup')
    nova_gazebo_dir = FindPackageShare('nova_gazebo')

    declared_arguments = [
        DeclareLaunchArgument(
            name='arm_params',
            default_value=PathJoinSubstitution([nova_bringup_dir, 'params', 'arm.yaml']),
            description='Absolute path to controllers params file',
        ),
        DeclareLaunchArgument(
            name='arm',
            default_value='true',
            description='whether to launch arm',
        ),
        DeclareLaunchArgument(
            name='world',
            default_value=PathJoinSubstitution([nova_gazebo_dir, 'worlds', 'arm_keyboard.sdf']),
            description='Full path to world model file to load',
        ),
        DeclareLaunchArgument(
            name='namespace',
            default_value='',
            description='Top-level namespace',
        ),
        DeclareLaunchArgument(
            name='rviz_params',
            default_value='arm_sim_cams.rviz',
            description='',
        ),
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )