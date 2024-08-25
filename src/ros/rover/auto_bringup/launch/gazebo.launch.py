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
    auto_bringup_dir = FindPackageShare('auto_bringup')
    rover_description_dir = FindPackageShare('rover_description')
    gazebo_dir = FindPackageShare('gazebo_ros')
    nova_gazebo_dir = FindPackageShare('nova_gazebo')

    pose = {'x': LaunchConfiguration('x_pose', default='-2.0'),
            'y': LaunchConfiguration('y_pose', default='-2.0'),
            'z': LaunchConfiguration('z_pose', default='0.05'),
            'R': LaunchConfiguration('roll', default='0.00'),
            'P': LaunchConfiguration('pitch', default='0.00'),
            'Y': LaunchConfiguration('yaw', default='0.00')}
    robot_name = LaunchConfiguration('robot_name')
    headless = LaunchConfiguration('headless')
    world = LaunchConfiguration('world')
    namespace = LaunchConfiguration('namespace')
    model = LaunchConfiguration('model')
    launch_robot_desciption = LaunchConfiguration('launch_robot_description')

    model_arg = DeclareLaunchArgument(name='model', default_value=PathJoinSubstitution([rover_description_dir, 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file')

    namespace_arg = DeclareLaunchArgument(
        'namespace',
        default_value='',
        description='Top-level namespace')

    headless_arg = DeclareLaunchArgument(
        'headless',
        default_value='False',
        description='Whether to execute gzclient)')

    robot_name_arg = DeclareLaunchArgument(
        'robot_name',
        default_value='Waratah',
        description='name of the robot')

    world_arg = DeclareLaunchArgument(
        'world',
        # TODO(orduno) Switch back once ROS argument passing has been fixed upstream
        #              https://github.com/ROBOTIS-GIT/turtlebot3_simulations/issues/91
        # default_value=os.path.join(get_package_share_directory('turtlebot3_gazebo'),
        # worlds/turtlebot3_worlds/waffle.model')
        default_value=PathJoinSubstitution([nova_gazebo_dir, "worlds", 'flat.model']),
        description='Full path to world model file to load')

    robot_description_arg = DeclareLaunchArgument(
        'launch_robot_description',
        default_value='True',
        description='Should gazebo launch its own robot description, or is one already running?'
    )

    urdf_launch_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'urdf.launch.py'])),
        condition=IfCondition(launch_robot_desciption),
        launch_arguments={"model": model, "gazebo": 'true'}.items()
    )

    start_gazebo_server_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution([gazebo_dir, 'launch', 'gzserver.launch.py'])),
        launch_arguments={"world": world}.items()
    )

    start_gazebo_client_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution([gazebo_dir, 'launch', 'gzclient.launch.py'])),
        condition=UnlessCondition(headless),
    )

    control_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'control.launch.py'])),
        launch_arguments={
            'gazebo': 'true',
        }.items()
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

    return LaunchDescription([
        namespace_arg,
        headless_arg,
        model_arg,
        robot_name_arg,
        world_arg,
        robot_description_arg,
        urdf_launch_cmd,
        start_gazebo_server_cmd,
        start_gazebo_client_cmd,
        spawn_rover_cmd,
        control_cmd,
    ])
