'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

General-purpose launch file to bring up all necessary
nodes for the rover's autonomous stack.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
INCLUDED LAUNCH FILES:
  - gazebo.launch.py
  - localization.launch.py
  - navigation.launch.py
  - lidar.launch.py
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	27/04/2023
EDITED:     05/01/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.substitutions import  PathJoinSubstitution, LaunchConfiguration, IfElseSubstitution, EnvironmentVariable
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

from os.path import expanduser, exists

def launch_setup(context, *args, **kwargs):
    # package directories
    local = LaunchConfiguration('local')

    auto_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/auto/auto_bringup']),
        FindPackageShare('auto_bringup')
    )
    nova_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/nova_bringup']),
        FindPackageShare('nova_bringup')
    )
    nova_gazebo_dir = FindPackageShare('nova_gazebo')

    comp = LaunchConfiguration('comp').perform(context).lower()
    
    # comp agnostic arguments
    autostart = LaunchConfiguration('autostart')
    controller_params = LaunchConfiguration('controller_params')
    localization = LaunchConfiguration('localization')
    log_level = LaunchConfiguration('log_level')
    map_params = LaunchConfiguration('map_params')
    model = LaunchConfiguration('model')
    namespace = LaunchConfiguration('namespace')
    navigation = LaunchConfiguration('navigation')
    release = LaunchConfiguration('release')
    rviz = LaunchConfiguration('rviz')
    rviz_params = LaunchConfiguration('rviz_params')
    use_respawn = LaunchConfiguration('use_respawn')
    lidar = LaunchConfiguration('lidar')
    fastlivo2_params = LaunchConfiguration('fastlivo2_params')
    mppi_config = LaunchConfiguration('mppi_config')

    # comp defaults
    if comp == 'arch':
        nav2_params_dir = PathJoinSubstitution([auto_bringup_dir, 'params', 'arch', 'nav2'])
        localization = 'False'
        rl_params = PathJoinSubstitution([auto_bringup_dir, 'params', 'arch', 'rl_arch.yaml'])
        world = PathJoinSubstitution([nova_gazebo_dir, 'worlds', 'auto_cubes.sdf'])
        gps = 'False'
    elif comp == 'urc':
        nav2_params_dir = PathJoinSubstitution([auto_bringup_dir, 'params', 'urc', 'nav2'])
        localization = 'True'
        rl_params = PathJoinSubstitution([auto_bringup_dir, 'params', 'urc', 'rl_urc.yaml'])
        world = PathJoinSubstitution([nova_gazebo_dir, 'worlds', 'urc_obstacles.sdf'])
        gps = 'True'
    else:
        raise ValueError('"comp" arg must be either "arch" or "urc"')
    
    # comp defaults overrides
    if LaunchConfiguration('localization').perform(context) != '':
        localization = LaunchConfiguration('localization')
    if LaunchConfiguration('rl_params').perform(context) != '':
        rl_params = LaunchConfiguration('rl_params')
    if LaunchConfiguration('world').perform(context) != '':
        world = LaunchConfiguration('world')
    if LaunchConfiguration('gps').perform(context) != '':
        gps = LaunchConfiguration('gps')

    return [
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'unity.launch.py'])),
            launch_arguments={
                'local': local,
                'comp': comp,
                'controller_params': controller_params,
                'model': model,
                'namespace': namespace,
                'world': 'ARCh2026',
                'release': release,
                'rviz': rviz,
                'rviz_params': rviz_params,
            }.items(),
        ),
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'realsense.launch.py'])),
            launch_arguments={
                'local': local,
                'comp': comp,
                'sim': 'True',
            }.items()
        ),
        IncludeLaunchDescription(
            condition=IfCondition(localization),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'localization.launch.py'])),
            launch_arguments={
                'local': local,
                'comp': comp,
                'sim': 'True',
                'gps': gps,
                'rl_params': rl_params,
            }.items()
        ),
        IncludeLaunchDescription(
            condition=IfCondition(navigation),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'navigation.launch.py'])),
            launch_arguments={
                'local': local,
                'comp': comp,
                'autostart': autostart,
                'container_name': 'nav2_container',
                'log_level': log_level,
                'namespace': namespace,
                'nav2_params_dir': nav2_params_dir,
                'use_respawn': use_respawn,
                'sim': 'True',
                'map_params': map_params,
                'mppi_config': mppi_config,
            }.items()
        ),
        IncludeLaunchDescription(
            condition=IfCondition(lidar),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'lidar.launch.py'])),
            launch_arguments={
                'local': local,
                'comp': comp,
                'fastlivo2_params': fastlivo2_params,
                'sim': 'True',
            }.items(),
        ),
    ]


def generate_launch_description():
    local = LaunchConfiguration('local')

    auto_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/auto/auto_bringup']),
        FindPackageShare('auto_bringup')
    )
    rover_description_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/rover_description']),
        FindPackageShare('rover_description')
    )
    drive_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/drive/drive_bringup']),
        FindPackageShare('drive_bringup')
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
            name='autostart',
            default_value='True',
            description='Automatically startup the nav2 stack',
        ),
        DeclareLaunchArgument(
            name='controller_params',
            default_value=PathJoinSubstitution([drive_bringup_dir, 'params', 'auto.yaml']),
            description='Absolute path to the auto drive controllers\' params file',
        ),
        DeclareLaunchArgument(
            name='log_level',
            default_value='info',
            description='What level of logging output should be displayed',
        ),
        DeclareLaunchArgument(
            name='map_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'arch', 'map.yaml']),
            description='Full path to the parameters file to use for static map layer',
        ),
        DeclareLaunchArgument(
            name='model',
            default_value=PathJoinSubstitution([rover_description_dir, 'banksia', 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file',
        ),
        DeclareLaunchArgument(
            name='namespace',
            default_value='',
            description='Top-level namespace',
        ),
        DeclareLaunchArgument(
            name='navigation',
            default_value='True',
            description='Flag to launch navigation stack',
        ),
        DeclareLaunchArgument( # Do not include 'rviz' argument in nested launch files https://github.com/ros2/launch/issues/313
            name='rviz',
            default_value='True',
            description='Flag to launch rviz',
        ),
        DeclareLaunchArgument(
            name='rviz_params',
            default_value='everything',
            description='Name of the rviz config file to use, without the .rviz extension. Must be located in src/ros/rover/auto/auto_bringup/rviz',
        ),
        DeclareLaunchArgument(
            name='use_respawn',
            default_value='False',
            description='Whether to respawn if a node crashes. Applied when composition is disabled.',
        ),
        DeclareLaunchArgument(
            name='lidar',
            default_value='True',
            description='Launch LiDAR nodes?',
        ),
        DeclareLaunchArgument(
            name='fastlivo2_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'fast_livo2', 'fastlivo2.yaml']),
            description='Params file for FAST-LIVO2 Nodes',
        ),
        DeclareLaunchArgument(
            name='mppi_config',
            default_value='regular',
            description='Name of the MPPI config to use (without .yaml)',
        ),
        # arguments with comp defaults
        DeclareLaunchArgument(
            name='localization',
            default_value='',
            description='Run robot_localization?',
        ),
        DeclareLaunchArgument(
            name='release',
            default_value='True',
            description='Use released build of Unity?',
        ),
        DeclareLaunchArgument(
            name='rl_params',
            default_value='',
            description='Full path to robot_localization parameters file',
        ),
        DeclareLaunchArgument(
            name='gps',
            default_value='',
            description='Fuse GPS?',
        ),
        DeclareLaunchArgument(
            name='world',
            default_value='',
            description='Full path to world model file to load',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
