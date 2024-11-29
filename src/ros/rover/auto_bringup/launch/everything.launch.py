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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from ament_index_python.packages import get_package_share_path, get_package_share_directory

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.substitutions import Command, FindExecutable, PathJoinSubstitution, LaunchConfiguration, AndSubstitution, NotSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare
import os

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = get_package_share_directory('auto_bringup')

    autostart = LaunchConfiguration('autostart')
    gazebo = LaunchConfiguration('gazebo')
<<<<<<< HEAD
=======
    headless = LaunchConfiguration('headless')
>>>>>>> a1ab665 (cleaned up launch files)
    localization = LaunchConfiguration('localization')
    log_level = LaunchConfiguration('log_level')
    namespace = LaunchConfiguration('namespace')
    navigation = LaunchConfiguration('navigation')
    params_file = LaunchConfiguration('params_file')
    rviz = LaunchConfiguration('rviz')
    use_respawn = LaunchConfiguration('use_respawn')
    world = LaunchConfiguration('world')

    return [
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'gazebo.launch.py'])),
            condition=IfCondition(gazebo),
            launch_arguments={
                'namespace': namespace, 
                'world': world, 
<<<<<<< HEAD
=======
                'headless': headless, 
>>>>>>> a1ab665 (cleaned up launch files)
            }.items(),
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'led.launch.py'])),
            condition=IfCondition(gazebo),
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'control.launch.py'])),
            condition=UnlessCondition(gazebo),
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'localization.launch.py'])),
            condition=IfCondition(localization),
            launch_arguments={
                'use_sim_time': gazebo,
                'load_map': localization,
            }.items()
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'rviz.launch.py'])),
            condition=IfCondition(rviz)
        ), 
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'navigation.launch.py'])),
            condition=IfCondition(navigation),
            launch_arguments={
                'namespace': namespace,
                'use_sim_time': gazebo,
                'autostart': autostart,
                'params_file': params_file,
                'use_respawn': use_respawn,
                'container_name': 'nav2_container',
                'log_level': log_level,
            }.items()
        ),
        Node(
                condition=IfCondition(gazebo),
                package='aruco_opencv',
                executable='aruco_tracker_autostart',
                arguments=['--ros-args', '--params-file', PathJoinSubstitution([auto_bringup_dir, 'params', 'aruco_tracker.yaml'])],
        ),
    ]

def generate_launch_description():
    auto_bringup_dir = get_package_share_directory('auto_bringup')
    gazebo_dir = get_package_share_directory('nova_gazebo')

    declared_arguments = [
        DeclareLaunchArgument(
            name='namespace',
            default_value='',
            description='Top-level namespace',
        ),
        DeclareLaunchArgument(
            name='world',
            default_value=PathJoinSubstitution([gazebo_dir, 'worlds', 'flat.sdf']),
            description='Full path to world model file to load',
        ),
        DeclareLaunchArgument(
            name='params_file',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2_params.yaml']),
            description='Full path to the ROS2 parameters file to use for all launched nodes',
        ),
        DeclareLaunchArgument(
            name='autostart', 
            default_value='True',
            description='Automatically startup the nav2 stack',
        ),
        DeclareLaunchArgument(
            name='use_respawn', 
            default_value='False',
            description='Whether to respawn if a node crashes. Applied when composition is disabled.',
        ),
        DeclareLaunchArgument( # Do not include 'rviz' argument in nested launch files https://github.com/ros2/launch/issues/313
            name='rviz',
            default_value='True',
            description='Flag to launch rviz',
        ),
        DeclareLaunchArgument(
            name='localization', 
            default_value='True',
            description='Flag to robot localization nodes',
        ),
        DeclareLaunchArgument(
            name='log_level', 
            default_value='info',
            description='What level of logging output should be displayed',
        ),
        DeclareLaunchArgument(
            name='gazebo', 
            default_value='True',
            description='Flag to launch gazebo',
        ),
        DeclareLaunchArgument(
            name='navigation', 
            default_value='True',
            description='Flag to launch navigation stack',
        ),
<<<<<<< HEAD
=======
        DeclareLaunchArgument(
            name='headless', 
            default_value='True',
            description='Flag to launch gazeboclient',
        ),
>>>>>>> a1ab665 (cleaned up launch files)
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
