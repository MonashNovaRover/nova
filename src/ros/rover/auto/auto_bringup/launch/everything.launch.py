'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

General-purpose launch file to bring up all necessary
nodes for the rover's autonomous stack.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
INCLUDED LAUNCH FILES:
  - gazebo.launch.py
  - drive.launch.py
  - localization.launch.py
  - rviz.launch.py
  - navigation.launch.py
  - rtabmap.launch.py
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	27/04/2023
EDITED:     05/01/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.substitutions import  PathJoinSubstitution, LaunchConfiguration
from launch.conditions import IfCondition, UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    # package directories
    auto_bringup_dir = FindPackageShare('auto_bringup')
    drive_bringup_dir = FindPackageShare('drive_bringup')
    nova_gazebo_dir = FindPackageShare('nova_gazebo')

    comp = LaunchConfiguration('comp').perform(context).lower()
    
    # comp agnostic arguments
    autostart = LaunchConfiguration('autostart')
    controller_params = LaunchConfiguration('controller_params')
    gazebo = LaunchConfiguration('gazebo')
    localization = LaunchConfiguration('localization')
    log_level = LaunchConfiguration('log_level')
    map_params = LaunchConfiguration('map_params')
    model = LaunchConfiguration('model')
    namespace = LaunchConfiguration('namespace')
    navigation = LaunchConfiguration('navigation')
    rviz = LaunchConfiguration('rviz')
    rviz_params = LaunchConfiguration('rviz_params')
    sim_params = LaunchConfiguration('sim_params')
    use_respawn = LaunchConfiguration('use_respawn')
    rtabmap = LaunchConfiguration('rtabmap')
    rtabmap_params = LaunchConfiguration('rtabmap_params')

    # comp defaults
    if comp == 'arch':
        nav2_params_dir = PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2_arch'])
        rl_params = PathJoinSubstitution([auto_bringup_dir, 'params', 'rl_arch.yaml'])
        world = PathJoinSubstitution([nova_gazebo_dir, 'worlds', 'auto_cubes.sdf'])
        gps = 'False'
    elif comp == 'urc':
        nav2_params_dir = PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2_urc'])
        rl_params = PathJoinSubstitution([auto_bringup_dir, 'params', 'rl_urc.yaml'])
        world = PathJoinSubstitution([nova_gazebo_dir, 'worlds', 'urc_obstacles.sdf'])
        gps = 'True'
    else:
        raise ValueError('"comp" arg must be either "arch" or "urc"')
    
    # comp defaults overrides
    if LaunchConfiguration('nav2_params_dir').perform(context) != '':
        nav2_params_dir = LaunchConfiguration('nav2_params_dir')
    if LaunchConfiguration('rl_params').perform(context) != '':
        rl_params = LaunchConfiguration('rl_params')
    if LaunchConfiguration('world').perform(context) != '':
        world = LaunchConfiguration('world')
    if LaunchConfiguration('gps').perform(context) != '':
        gps = LaunchConfiguration('gps')

    return [
        IncludeLaunchDescription(
            condition=IfCondition(gazebo),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'gazebo.launch.py'])),
            launch_arguments={
                'comp': comp,
                'camera':'True',
                'controller_params': controller_params,
                'model': model,
                'namespace': namespace,
                'world': world,
            }.items(),
        ),
        IncludeLaunchDescription(
            condition=UnlessCondition(gazebo),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([drive_bringup_dir, 'launch', 'drive.launch.py'])),
        ),
        IncludeLaunchDescription(
            condition=IfCondition(localization),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'localization.launch.py'])),
            launch_arguments={
                'comp': comp,
                'gazebo': gazebo,
                'gps': gps,
                'rl_params': rl_params,
            }.items()
        ),
        IncludeLaunchDescription(
            condition=IfCondition(rviz),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'rviz.launch.py'])),
            launch_arguments={
                'gazebo': gazebo,
                'rviz_params': rviz_params,
            }.items()
        ),
        IncludeLaunchDescription(
            condition=IfCondition(navigation),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'navigation.launch.py'])),
            launch_arguments={
                'comp': comp,
                'autostart': autostart,
                'container_name': 'nav2_container',
                'log_level': log_level,
                'namespace': namespace,
                'nav2_params_dir': nav2_params_dir,
                'sim_params': sim_params,
                'use_respawn': use_respawn,
                'gazebo': gazebo,
                'map_params': map_params,
            }.items()
        ),
        IncludeLaunchDescription(
            condition=IfCondition(rtabmap),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'rtabmap.launch.py'])),
            launch_arguments={
                'rtabmap_params': rtabmap_params,
                'gazebo': gazebo,
            }.items(),
        ),
    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')
    rover_description_dir = FindPackageShare('rover_description')
    drive_bringup_dir = FindPackageShare('drive_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='comp',
            default_value='arch',
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
            name='gazebo',
            default_value='True',
            description='Flag to launch gazebo',
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
            name='map_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'map.yaml']),
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
        DeclareLaunchArgument( # Do not include 'rviz' argument in nested launch files https://github.com/ros2/launch/issues/313
            name='rviz_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'rviz', 'navigation.rviz']),
            description='Full path to the RViz config file to use',
        ),
        DeclareLaunchArgument(
            name='sim_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2_sim.yaml']),
            description='Sim parameters to use if using sim time', 
        ),
        DeclareLaunchArgument(
            name='use_respawn',
            default_value='False',
            description='Whether to respawn if a node crashes. Applied when composition is disabled.',
        ),
        DeclareLaunchArgument(
            name='rtabmap',
            default_value='True',
            description='Launch rtabmap?',
        ),
        DeclareLaunchArgument(
            name='rtabmap_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'rtabmap.yaml']),
            description='Params file for RTABMap Nodes',
        ),
        # arguments with comp defaults
        DeclareLaunchArgument(
            name='nav2_params_dir',
            default_value='',
            description='Full path to the folder with ROS2 parameters files to use with all nodes',
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
