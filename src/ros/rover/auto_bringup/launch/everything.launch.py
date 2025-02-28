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
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.substitutions import  PathJoinSubstitution, LaunchConfiguration
from launch.conditions import IfCondition, UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')

    autostart = LaunchConfiguration('autostart')
    controllers = LaunchConfiguration('controllers')
    gazebo = LaunchConfiguration('gazebo')
    localization = LaunchConfiguration('localization')
    log_level = LaunchConfiguration('log_level')
    model = LaunchConfiguration('model')
    namespace = LaunchConfiguration('namespace')
    nav2_params = LaunchConfiguration('nav2_params')
    navigation = LaunchConfiguration('navigation')
    rviz = LaunchConfiguration('rviz')
    rviz_config = LaunchConfiguration('rviz_config')
    use_respawn = LaunchConfiguration('use_respawn')
    world = LaunchConfiguration('world')

    return [
        IncludeLaunchDescription(
            condition=IfCondition(gazebo),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'gazebo.launch.py'])),
            launch_arguments={
                'camera':'True',
                'controllers': controllers,
                'model': model,
                'namespace': namespace,
                'world': world,
            }.items(),
        ),
        IncludeLaunchDescription(
            condition=UnlessCondition(gazebo),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'control.launch.py'])),
        ),
        IncludeLaunchDescription(
            condition=IfCondition(localization),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'localization.launch.py'])),
            launch_arguments={
                'load_map': localization,
                'use_sim_time': gazebo,
            }.items()
        ),
        IncludeLaunchDescription(
            condition=IfCondition(rviz),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'rviz.launch.py'])),
            launch_arguments={
                'use_sim_time': gazebo,
                'config': rviz_config,
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
                'params_file': nav2_params,
                'use_respawn': use_respawn,
                'use_sim_time': gazebo,
            }.items()
        ),
    ]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')
    nova_gazebo_dir = FindPackageShare('nova_gazebo')
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [
        DeclareLaunchArgument(
            name='autostart',
            default_value='True',
            description='Automatically startup the nav2 stack',
        ),
        DeclareLaunchArgument(
            name='controllers',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'controllers.yaml']),
            description='Absolute path to controller params file',
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
            name='nav2_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2.yaml']),
            description='Full path to the ROS2 parameters file to use for all launched nodes',
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
            name='rviz_config',
            default_value='navigation.rviz',
            description='RViz configuration file',
        ),
        DeclareLaunchArgument(
            name='use_respawn',
            default_value='False',
            description='Whether to respawn if a node crashes. Applied when composition is disabled.',
        ),
        DeclareLaunchArgument(
            name='world',
            default_value=PathJoinSubstitution([nova_gazebo_dir, 'worlds', 'auto.sdf']),
            description='Full path to world model file to load',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
