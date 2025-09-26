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
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction, TimerAction
from launch.substitutions import  PathJoinSubstitution, LaunchConfiguration
from launch.conditions import IfCondition, UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')
    drive_bringup_dir = FindPackageShare('drive_bringup')

    autostart = LaunchConfiguration('autostart')
    controller_params = LaunchConfiguration('controller_params')
    gazebo = LaunchConfiguration('gazebo')
    gps = LaunchConfiguration('gps')
    localization = LaunchConfiguration('localization')
    log_level = LaunchConfiguration('log_level')
    map_params = LaunchConfiguration('map_params')
    model = LaunchConfiguration('model')
    namespace = LaunchConfiguration('namespace')
    nav2_params_dir = LaunchConfiguration('nav2_params_dir')
    navigation = LaunchConfiguration('navigation')
    rl_params = LaunchConfiguration('rl_params')
    rviz = LaunchConfiguration('rviz')
    rviz_params = LaunchConfiguration('rviz_params')
    sim_params = LaunchConfiguration('sim_params')
    use_respawn = LaunchConfiguration('use_respawn')
    world = LaunchConfiguration('world')
    rtabmap = LaunchConfiguration('rtabmap')

    auto_bringup_common_nodes = GroupAction([
        IncludeLaunchDescription(
            condition=IfCondition(gazebo),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'gazebo.launch.py'])),
            launch_arguments={
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
                'autostart': autostart,
                'container_name': 'nav2_container',
                'log_level': log_level,
                'namespace': namespace,
                'nav2_params_dir': nav2_params_dir,
                'sim_params': sim_params,
                'use_respawn': use_respawn,
                'use_sim_time': gazebo,
                'map_params': map_params,
            }.items()
        ),
    ])
    return [
        IncludeLaunchDescription(
            condition = IfCondition(rtabmap),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'rtabmap.launch.py'])),
            launch_arguments={
                'pointclouds':'False',
                # 'gazebo': gazebo,
            }.items()
        ),
        TimerAction(
            period = 10.0, #Delay in seconds
            actions = [auto_bringup_common_nodes],
        ),
    ]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')
    nova_gazebo_dir = FindPackageShare('nova_gazebo')
    rover_description_dir = FindPackageShare('rover_description')
    drive_bringup_dir = FindPackageShare('drive_bringup')

    declared_arguments = [
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
            name='gps',
            default_value='True',
            description='Fuse GPS?',
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
            name='nav2_params_dir',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2_urc']),
            description='Full path to the folder with ROS2 parameters files to use with all nodes',
        ),
        DeclareLaunchArgument(
            name='navigation',
            default_value='True',
            description='Flag to launch navigation stack',
        ),
        DeclareLaunchArgument(
            name='rl_params',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','rl_urc.yaml']),
            description='',
        ),
        DeclareLaunchArgument( # Do not include 'rviz' argument in nested launch files https://github.com/ros2/launch/issues/313
            name='rviz',
            default_value='True',
            description='Flag to launch rviz',
        ),
        DeclareLaunchArgument( # Do not include 'rviz' argument in nested launch files https://github.com/ros2/launch/issues/313
            name='rviz_params',
            default_value='navigation.rviz',
            description='RViz configuration file',
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
            name='world',
            default_value=PathJoinSubstitution([nova_gazebo_dir, 'worlds', 'urc_obstacles.sdf']),
            description='Full path to world model file to load',
        ),
        DeclareLaunchArgument(
            name='rtabmap',
            default_value='False',
            description='Launch rtabmap?',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
