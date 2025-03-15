'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the base station to start all
    base station scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - control/inputs/inputs_publisher     [inputs]
  - electronics/electronics/radio_monitor.py
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CREATION:   15/12/2021
EDITED:     04/02/2025
EDITED BY: Max Tory, Taaj Street, Dylan Gonzalez
    Victor Bartlinski
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.conditions import IfCondition
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    nova_bringup_dir = FindPackageShare('nova_bringup')

    arm = LaunchConfiguration('arm')
    arm_urdf_path = LaunchConfiguration('arm_urdf_path')
    gazebo = LaunchConfiguration('gazebo')
    rover = LaunchConfiguration('rover')
    urdf = LaunchConfiguration('urdf')

    return [
        Node(
            package='inputs',
            executable='inputs_publisher',
            output='screen',
            emulate_tty=True,
            parameters=[{'use_sim_time': gazebo}],
        ),
        IncludeLaunchDescription(
            condition=IfCondition(urdf),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([nova_bringup_dir, 'launch', 'urdf.launch.py'])),
            launch_arguments = {
                'arm_urdf_path': arm_urdf_path,
                'arm': arm,
                'rover': rover,
            }.items(),
        ),
    ]

def generate_launch_description():
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [
        DeclareLaunchArgument(
            name='arm', 
            default_value='True',
            description='Include arm URDF in robot_description?',
        ),
        DeclareLaunchArgument(
            name='arm_urdf_path', 
            default_value = PathJoinSubstitution([rover_description_dir, 'waratah_arm', 'urdf', 'arm.urdf.xacro']), 
            description='Absolute path to arm urdf file',
        ),
        DeclareLaunchArgument(
            name='gazebo', 
            default_value='False',
            description='',
        ),
        DeclareLaunchArgument(
            name='rover', 
            default_value='True',
            description='Include rover URDF in robot_description?',
        ),
        DeclareLaunchArgument(
            name='urdf', 
            default_value='False',
            description='Publish robot_description?',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
