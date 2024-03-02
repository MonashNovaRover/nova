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
    core_dir = get_package_share_directory('core')

    # Launch Configurations
    namespace = LaunchConfiguration('namespace')
    world = LaunchConfiguration('world')
    params_file = LaunchConfiguration('params_file')
    autostart = LaunchConfiguration('autostart')
    use_composition = LaunchConfiguration('use_composition')
    use_respawn = LaunchConfiguration('use_respawn')
    use_real_odometry = LaunchConfiguration('use_real_odometry')
    localization = LaunchConfiguration('localization')
    load_map = LaunchConfiguration('load_map')
    log_level = LaunchConfiguration('log_level')
    gazebo = LaunchConfiguration('gazebo')
    autonomous = LaunchConfiguration('autonomous')
    headless = LaunchConfiguration('headless')
    wheel_odom_only = LaunchConfiguration('wheel_odom_only')
    rviz = LaunchConfiguration('launch_rviz')

    # Launch Arguments
    namespace_arg = DeclareLaunchArgument(
        'namespace',
        default_value='',
        description='Top-level namespace')

    namespace_arg = DeclareLaunchArgument(
        'namespace',
        default_value='',
        description='Top-level namespace')

    world_arg = DeclareLaunchArgument(
        'world',
        # TODO(orduno) Switch back once ROS argument passing has been fixed upstream
        #              https://github.com/ROBOTIS-GIT/turtlebot3_simulations/issues/91
        # default_value=os.path.join(get_package_share_directory('turtlebot3_gazebo'),
        # worlds/turtlebot3_worlds/waffle.model')
        default_value=PathJoinSubstitution([core_dir, "worlds", 'flat.model']),
        description='Full path to world model file to load')

    params_file_arg = DeclareLaunchArgument(
        'params_file',
        default_value=os.path.join(core_dir, 'params', 'nav2_params.yaml'),
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

    use_real_odom_arg = DeclareLaunchArgument(
        'use_real_odometry',
        default_value='true',
        description='True to use robot_localization odometry, False to use p3d gazebo plugin'
    )

    localization_arg = DeclareLaunchArgument(
        'localization',
        default_value='True',
        description='Flag for running localisation'
    )

    wheel_odom_only_arg = DeclareLaunchArgument(
        'wheel_odom_only',
        default_value='false',
        description='Flag to launch with wheel odometry as the only localization method'
    )
    
    rviz_arg = DeclareLaunchArgument(
        'launch_rviz',
        default_value='True',
        description='Flag to launch rviz'
    )

    load_map_arg = DeclareLaunchArgument(
        'load_map',
        default_value='False',
        description='Command for RTAB Map to load an existing map file and localize in it'
    )

    log_level_arg = DeclareLaunchArgument(
        'log_level', default_value='info',
        description='What level of logging output should be displayed')

    gazebo_arg = DeclareLaunchArgument(
        'gazebo', default_value='True',
        description='Should we simulate in gazebo or do we have rover hardware?')

    autonomous_arg = DeclareLaunchArgument(
        'autonomous', default_value='True',
        description='Should we launch navigation2 and other autonomy code?')

    headless_arg = DeclareLaunchArgument(
        'headless', default_value="True",
        description="Should gazebo launch with client"
    )

    # Include other launch files
    gazebo_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(core_dir, 'launch', 'gazebo_launch.py')),
        condition=IfCondition(gazebo),
        launch_arguments={
            'namespace': namespace,
            'world': world,
            'headless': headless,
            'launch_robot_description': 'True'
        }.items()
    )

    localization_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(core_dir, 'launch', 'localization_launch.py')),
        launch_arguments={
            'use_sim_time': gazebo,
            'use_real_odometry': use_real_odometry,
            'load_map': localization,
        }.items(),
        condition=IfCondition(AndSubstitution(localization, NotSubstitution(wheel_odom_only)))
    )

    wheel_odom_localization_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(core_dir, 'launch', 'wheel_odom_localization_launch.py')),
        launch_arguments={
            'use_sim_time': gazebo,
        }.items(),
        condition = IfCondition(AndSubstitution(localization, wheel_odom_only))
    )

    control_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(core_dir, 'launch', 'control_launch.py')),
        launch_arguments={
            'gazebo': gazebo,
        }.items()
    )

    rviz_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(core_dir, 'launch', 'rviz_launch.py')),
        condition=IfCondition(rviz)
    )

    navigation_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(core_dir, 'launch', 'navigation_launch.py')),
        condition=IfCondition(autonomous),
        launch_arguments={
            'namespace': namespace,
            'use_sim_time': gazebo,
            'autostart': autostart,
            'params_file': params_file,
            'use_composition': use_composition,
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
        use_real_odom_arg,
        localization_arg,
        wheel_odom_only_arg,
        load_map_arg,
        log_level_arg,
        gazebo_arg,
        autonomous_arg,
        headless_arg,
        rviz_arg,
        gazebo_cmd,
        localization_cmd,
        wheel_odom_localization_cmd,
        control_cmd,
        rviz_cmd,
        navigation_cmd,
    ])
