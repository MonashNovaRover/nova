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

# Generate the launch file with all inputs
def generate_launch_description():
    core_path = get_package_share_path('core')
    bringup_dir = get_package_share_directory('nova_nav2_bringup')
    default_model_path = core_path / 'urdf/rover.urdf.xacro'

    namespace = LaunchConfiguration('namespace')
    pose = {'x': LaunchConfiguration('x_pose', default='-2.00'),
            'y': LaunchConfiguration('y_pose', default='-0.50'),
            'z': LaunchConfiguration('z_pose', default='5.01'),
            'R': LaunchConfiguration('roll', default='0.00'),
            'P': LaunchConfiguration('pitch', default='0.00'),
            'Y': LaunchConfiguration('yaw', default='0.00')}
    robot_name = LaunchConfiguration('robot_name')
    headless = LaunchConfiguration('headless')
    world = LaunchConfiguration('world')

    model_arg = DeclareLaunchArgument(name='model', default_value=str(default_model_path),
            description='Absolute path to robot urdf file')

    namespace_arg = DeclareLaunchArgument(
        'namespace',
        default_value='',
        description='Top-level namespace')

    headless_arg = DeclareLaunchArgument(
        'headless',
        default_value='True',
        description='Whether to execute gzclient)')

    world_arg = DeclareLaunchArgument(
        'world',
        # TODO(orduno) Switch back once ROS argument passing has been fixed upstream
        #              https://github.com/ROBOTIS-GIT/turtlebot3_simulations/issues/91
        # default_value=os.path.join(get_package_share_directory('turtlebot3_gazebo'),
        # worlds/turtlebot3_worlds/waffle.model')
        default_value=PathJoinSubstitution([bringup_dir, 'worlds', 'model.sdf']),
        description='Full path to world model file to load')

    robot_description = ParameterValue(Command(['xacro ', LaunchConfiguration('model')]),
                                       value_type=str)

    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description}]
    )

    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            [PathJoinSubstitution([FindPackageShare("gazebo_ros"), "launch", "gazebo.launch.py"])]
        ),
        launch_arguments={"verbose": "false"}.items(),
    )

    gazebo_dir = get_package_share_directory('gazebo_ros')
    # Specify the actions
    start_gazebo_server_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([gazebo_dir, '/launch/gzserver.launch.py']),
        launch_arguments={"world": world}.items()
    )

    start_gazebo_client_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([gazebo_dir, '/launch/gzclient.launch.py']),
        condition=UnlessCondition(headless),
    )

    spawn_entity = Node(
        package='gazebo_ros',
        executable='spawn_entity.py',
        arguments=["-topic", "robot_description", "-entity", "Warratah"]
    )
    start_gazebo_spawner_cmd = Node(
        package='gazebo_ros',
        executable='spawn_entity.py',
        output='screen',
        arguments=[
            '-topic', 'robot_description',
            '-entity', robot_name,
            '-robot_namespace', namespace,
            '-x', pose['x'], '-y', pose['y'], '-z', pose['z'],
            '-R', pose['R'], '-P', pose['P'], '-Y', pose['Y']])


    '''
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
    '''

    four_wheel_steering_controller = Node(
        package="controller_manager",
        executable="spawner",
        arguments=["four_wheel_steering_controller"]
    )

    return LaunchDescription([
        model_arg,
        namespace_arg,
        headless_arg,
        world_arg,
        robot_state_publisher_node,
        start_gazebo_server_cmd,
        start_gazebo_client_cmd,
        #gazebo,
        spawn_entity,
        #wheel_velocity_controller,
        # pivot_position_controller,
        #pivot_joint_trajectory_controller,
        four_wheel_steering_controller,
    ])
