'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code to simulate the rover in Unity.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	08/07/2026
EDITED:     08/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, ExecuteProcess
from launch.conditions import IfCondition
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration, IfElseSubstitution, EnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

from os.path import expanduser

def launch_setup(context, *args, **kwargs):
    # package directories
    local = LaunchConfiguration('local')

    auto_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/auto/auto_bringup']),
        FindPackageShare('auto_bringup')
    )
    drive_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/drive/drive_bringup']),
        FindPackageShare('drive_bringup')
    )

    comp = LaunchConfiguration('comp').perform(context).lower()

    # comp agnostic arguments
    log_level = LaunchConfiguration('log_level')
    model = LaunchConfiguration('model')
    robot_name = LaunchConfiguration('robot_name')
    robot_type = LaunchConfiguration('robot_type')
    world = LaunchConfiguration('world')
    rviz = LaunchConfiguration('rviz')
    rviz_params = LaunchConfiguration('rviz_params')

    # comp defaults
    if comp == 'arch':
        world = 'ARCh2026'
    elif comp == 'urc':
        world = 'ARCh2026'
    else:
        raise ValueError('"comp" arg must be either "arch" or "urc"')

    # comp defaults overrides
    if LaunchConfiguration('world').perform(context) != '':
        world = LaunchConfiguration('world')

    return [
        # Start ROS2 Controllers
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([drive_bringup_dir, 'launch', 'drive.launch.py'])),
            launch_arguments={'local': local, 'auto': 'True', 'sim': 'True'}.items(),
        ),
        # Connect to Unity
        Node(
            package='ros_tcp_endpoint',
            executable='default_server_endpoint',
        ),
        Node(
            package='image_transport',
            executable='republish',
            parameters=[{
                'in_transport': 'compressed',
                'out_transport': 'raw',
            }],
            remappings=[
                ('in/compressed', '/d415/color/image_raw/compressed'),
                ('out', '/d415/color/image_raw'),
            ],
        ),
        # Run unity-sim
        ExecuteProcess(
            cmd=[
                'nova-unity-sim', 
                '-screen-fullscreen', '0', 
                '-window-mode', 'windowed', 
                ['scene=', world], 
                ['robot=', robot_type]],
            output='screen',
        ),
    ]

def generate_launch_description():
    local = LaunchConfiguration('local')

    rover_description_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/rover_description']),
        FindPackageShare('rover_description')
    )

    declared_arguments = [
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use local directories instead of the nix store.',
        ),
        DeclareLaunchArgument(
            name='comp',
            default_value=EnvironmentVariable('COMP', default_value='URC'),
            description='ARCh or URC',
        ),
        # comp agnostic arguments
        DeclareLaunchArgument(
            name='log_level',
            default_value='info',
            description='What level of logging output should be displayed',
        ),
        DeclareLaunchArgument(
            name='model',
            default_value=PathJoinSubstitution([rover_description_dir, 'banksia', 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file',
        ),
        DeclareLaunchArgument(
            name='robot_name',
            default_value='Banksia',
            description='name of the robot',
        ),
        DeclareLaunchArgument(
            name='robot_type',
            default_value='auto',
            description='type of the robot, e.g. default, auto, arm',
        ),
        DeclareLaunchArgument(
            name='rviz',
            default_value='False',
            description='Whether to launch RViz',
        ),
        DeclareLaunchArgument( # Do not include 'rviz' argument in nested launch files https://github.com/ros2/launch/issues/313
            name='rviz_params',
            default_value='everything',
            description='Name of the rviz config file to use, without the .rviz extension. Must be located in src/ros/rover/auto/auto_bringup/rviz',
        ),
        # arguments with comp defaults
        DeclareLaunchArgument(
            name='world',
            default_value='ARC2025',
            description='Full path to world model file to load',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
