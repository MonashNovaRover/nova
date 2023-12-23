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
from launch.substitutions import Command, FindExecutable, PathJoinSubstitution, LaunchConfiguration
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
    gazebo_dir = get_package_share_directory('gazebo_ros')
    default_model_path = os.path.join(core_dir, 'urdf', 'rover.urdf.xacro')

    # Launch Configurations
    namespace = LaunchConfiguration('namespace')
    use_sim_time = LaunchConfiguration('use_sim_time')
    params_file = LaunchConfiguration('params_file')
    autostart = LaunchConfiguration('autostart')
    use_composition = LaunchConfiguration('use_composition')
    use_respawn = LaunchConfiguration('use_respawn')
    pose = {'x': LaunchConfiguration('x_pose', default='-2.00'),
            'y': LaunchConfiguration('y_pose', default='-0.50'),
            'z': LaunchConfiguration('z_pose', default='0.01'),
            'R': LaunchConfiguration('roll', default='0.00'),
            'P': LaunchConfiguration('pitch', default='0.00'),
            'Y': LaunchConfiguration('yaw', default='0.00')}
    robot_name = LaunchConfiguration('robot_name')
    headless = LaunchConfiguration('headless')
    world = LaunchConfiguration('world')
    log_level = LaunchConfiguration('log_level')
    #world_name = LaunchConfiguration('world_name')

    # Launch arguments
    model_arg = DeclareLaunchArgument(name='model', default_value=str(default_model_path),
            description='Absolute path to robot urdf file')

    robot_name_arg = DeclareLaunchArgument(
        'robot_name',
        default_value='Waratah',
        description='name of the robot')

    namespace_arg = DeclareLaunchArgument(
        'namespace',
        default_value='',
        description='Top-level namespace')

    headless_arg = DeclareLaunchArgument(
        'headless',
        default_value='True',
        description='Whether to execute gzclient)')

    # world_name_arg = DeclareLaunchArgument(
    #     'world_name',
    #     # TODO(orduno) Switch back once ROS argument passing has been fixed upstream
    #     #              https://github.com/ROBOTIS-GIT/turtlebot3_simulations/issues/91
    #     # default_value=os.path.join(get_package_share_directory('turtlebot3_gazebo'),
    #     # worlds/turtlebot3_worlds/waffle.model')
    #     default_value='urc_er.model',
    #     description='Full path to world model file to load')

    world_arg = DeclareLaunchArgument(
        'world',
        # TODO(orduno) Switch back once ROS argument passing has been fixed upstream
        #              https://github.com/ROBOTIS-GIT/turtlebot3_simulations/issues/91
        # default_value=os.path.join(get_package_share_directory('turtlebot3_gazebo'),
        # worlds/turtlebot3_worlds/waffle.model')
        default_value=PathJoinSubstitution([core_dir, "worlds", 'urc_er.model']),
        description='Full path to world model file to load')

    # Declare the launch arguments
    declare_namespace_arg = DeclareLaunchArgument(
        'namespace',
        default_value='',
        description='Top-level namespace')

    declare_use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation (Gazebo) clock if true')

    declare_params_file_arg = DeclareLaunchArgument(
        'params_file',
        default_value=os.path.join(core_dir, 'params', 'nav2_params.yaml'),
        description='Full path to the ROS2 parameters file to use for all launched nodes')

    declare_autostart_arg = DeclareLaunchArgument(
        'autostart', default_value='true',
        description='Automatically startup the nav2 stack')

    declare_use_composition_arg = DeclareLaunchArgument(
        'use_composition', default_value='False',
        description='Whether to use composed bringup')

    declare_use_respawn_arg = DeclareLaunchArgument(
        'use_respawn', default_value='False',
        description='Whether to respawn if a node crashes. Applied when composition is disabled.')

    log_level_arg = DeclareLaunchArgument(
        'log_level', default_value='info',
        description='What level of logging output should be displayed')

    robot_description = ParameterValue(Command(['xacro ', LaunchConfiguration('model')]),
                                       value_type=str)

    # Executables
    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description}]
    )

    start_gazebo_server_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([gazebo_dir, '/launch/gzserver.launch.py']),
        launch_arguments={"world": world}.items()
    )

    start_gazebo_client_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([gazebo_dir, '/launch/gzclient.launch.py']),
        condition=UnlessCondition(headless),
    )

    spawn_rover_cmd = Node(
        package='gazebo_ros',
        executable='spawn_entity.py',
        output='screen',
        arguments=[
            '-topic', 'robot_description',
            '-entity', robot_name,
            '-robot_namespace', namespace,
            '-x', pose['x'], '-y', pose['y'], '-z', pose['z'],
            '-R', pose['R'], '-P', pose['P'], '-Y', pose['Y']])

    wheel_velocity_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["wheel_velocity_controller"]
    )

    pivot_joint_trajectory_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["pivot_joint_trajectory_controller"]
    )

    joint_broad = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["joint_broad"]
    )

    ukf_localisation = Node(
        package='robot_localization',
        executable='ukf_node',
        name='ukf_filter_node',
        output='screen',
        parameters=[(get_package_share_path("core") / 'params' / 'ukf.yaml').as_posix()],
    )

    # TODO: Implement real SLAM rather than publishing a static transform. This is currently necessary for things to be visible in the RVIZ 'map' frame
    slam_cmd = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        arguments = ['--x', '0', '--y', '0', '--z', '0', '--yaw', '0', '--pitch', '0', '--roll', '0', '--frame-id', 'map', '--child-frame-id', 'odom']
    )

    navigation_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(core_dir, 'launch', 'navigation_launch.py')),
        launch_arguments={
            'namespace': namespace,
            'use_sim_time': use_sim_time,
            'autostart': autostart,
            'params_file': params_file,
            'use_composition': use_composition,
            'use_respawn': use_respawn,
            'container_name': 'nav2_container',
            'log_level': log_level,
        }.items()
    )

    return LaunchDescription([
        model_arg,
        robot_name_arg,
        namespace_arg,
        headless_arg,
        #world_name_arg,
        world_arg,
        declare_namespace_arg,
        declare_use_sim_time_arg,
        declare_params_file_arg,
        declare_autostart_arg,
        declare_use_composition_arg,
        declare_use_respawn_arg,
        log_level_arg,
        start_gazebo_server_cmd,
        start_gazebo_client_cmd,
        spawn_rover_cmd,
        robot_state_publisher_node,
        wheel_velocity_controller,
        ukf_localisation,
        slam_cmd,
        pivot_joint_trajectory_controller,
        navigation_cmd,
        joint_broad,
    ])
