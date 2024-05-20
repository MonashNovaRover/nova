"""
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
"""

from ament_index_python.packages import get_package_share_path, get_package_share_directory

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.substitutions import Command, FindExecutable, PathJoinSubstitution, LaunchConfiguration, AndSubstitution, NotSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare
import os

# Generate the launch file with all inputs
def generate_launch_description():
    # Useful paths
    auto_bringup_dir = FindPackageShare('auto_bringup')

    # Launch Configurations
    namespace = LaunchConfiguration('namespace')
    world = LaunchConfiguration('world')
    params_file = LaunchConfiguration('nav2_params_file')
    autostart = LaunchConfiguration('autostart')
    use_respawn = LaunchConfiguration('use_respawn')
    localization = LaunchConfiguration('localization')
    log_level = LaunchConfiguration('log_level')
    gazebo = LaunchConfiguration('gazebo')
    navigation = LaunchConfiguration('navigation')
    headless = LaunchConfiguration('headless')
    rviz = LaunchConfiguration('launch_rviz')

    # Launch Arguments
    namespace_arg = DeclareLaunchArgument(
        'namespace',
        default_value='',
        description='Top-level namespace')

    world_arg = DeclareLaunchArgument(
        'world',
        # TODO(orduno) Switch back once ROS argument passing has been fixed upstream
        #              https://github.com/ROBOTIS-GIT/turtlebot3_simulations/issues/91
        # default_value=PathJoinSubstitution([get_package_share_directory('turtlebot3_gazebo'),
        # worlds/turtlebot3_worlds/waffle.model')
        default_value=PathJoinSubstitution([FindPackageShare('nova_gazebo'), "worlds", 'flat.model']),
        description='Full path to world model file to load')

    params_file_arg = DeclareLaunchArgument(
        'nav2_params_file',
        default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'nav2_params.yaml']),
        description='Full path to the ROS2 parameters file to use for all launched nodes')

    autostart_arg = DeclareLaunchArgument(
        'autostart', default_value='true',
        description='Automatically startup the nav2 stack')

    use_composition_arg = DeclareLaunchArgument(
        'use_composition', default_value='False',
        description='Whether to use composed bringup')

    use_respawn_arg = DeclareLaunchArgument(
        'use_respawn', default_value='False',
        description='Whether to respawn if a node crashes. Applied when composition is disabled.')

    rviz_arg = DeclareLaunchArgument(
        'launch_rviz',
        default_value='True',
        description='Flag to launch rviz'
    )

    localization_arg = DeclareLaunchArgument(
        'localization', default_value='True',
        description='Flag to robot localization nodes')

    log_level_arg = DeclareLaunchArgument(
        'log_level', default_value='info',
        description='What level of logging output should be displayed')

    gazebo_arg = DeclareLaunchArgument(
        'gazebo', default_value='True',
        description='Flag to launch gazebo')

    navigation_arg = DeclareLaunchArgument(
        'navigation', default_value='True',
        description='Flag to launch navigation stack')

    headless_arg = DeclareLaunchArgument(
        'headless', default_value="True",
        description="Flag to launch gazeboclient"
    )

    # Include other launch files
    gazebo_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'gazebo.launch.py'])),
        condition=IfCondition(gazebo),
        launch_arguments={
            'namespace': namespace,
            'world': world,
            'headless': headless,
        }.items()
    )

    localization_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'localization.launch.py'])),
        condition=IfCondition(localization),
        launch_arguments={
            'use_sim_time': gazebo,
            'load_map': localization,
        }.items()
    )

    control_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'control.launch.py'])),
        condition=UnlessCondition(gazebo),
    )

    rviz_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'rviz.launch.py'])),
        condition=IfCondition(rviz)
    )

    navigation_cmd = IncludeLaunchDescription(
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
    )

    return LaunchDescription([
        namespace_arg,
        namespace_arg,
        world_arg,
        gazebo_arg,
        params_file_arg,
        autostart_arg,
        use_composition_arg,
        use_respawn_arg,
        localization_arg,
        log_level_arg,
        gazebo_arg,
        navigation_arg,
        headless_arg,
        rviz_arg,
        gazebo_cmd,
        localization_cmd,
        control_cmd,
        rviz_cmd,
        navigation_cmd,
    ])
