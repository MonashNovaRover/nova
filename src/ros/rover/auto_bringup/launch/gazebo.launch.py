
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
PACKAGE: 	core
CREATION:	27/04/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.conditions import IfCondition
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, AppendEnvironmentVariable, OpaqueFunction
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')
    nova_gazebo_dir = FindPackageShare('nova_gazebo')
    ros_gz_sim_dir = FindPackageShare('ros_gz_sim')

    angle = LaunchConfiguration('angle')
    camera = LaunchConfiguration('camera')
    gz_params = LaunchConfiguration('gz_params')
    controllers = LaunchConfiguration('controllers')
    model = LaunchConfiguration('model')
    namespace = LaunchConfiguration('namespace')
    pose = {'x': LaunchConfiguration('x').perform(context),
            'y': LaunchConfiguration('y').perform(context),
            'z': LaunchConfiguration('z').perform(context),
            'R': LaunchConfiguration('R').perform(context),
            'P': LaunchConfiguration('P').perform(context),
            'Y': LaunchConfiguration('Y').perform(context)}
    robot_name = LaunchConfiguration('robot_name')
    world = LaunchConfiguration('world')

    return [
        AppendEnvironmentVariable(
            name='GZ_SIM_RESOURCE_PATH',
            value=PathJoinSubstitution([nova_gazebo_dir, 'models'])
        ),
        AppendEnvironmentVariable(
            name='GZ_SIM_RESOURCE_PATH',
            value=PathJoinSubstitution([nova_gazebo_dir, 'worlds'])
        ),
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'control.launch.py'])),
            launch_arguments={'controllers': controllers, 'gazebo': 'True'}.items(),
        ),
        IncludeLaunchDescription(
            condition=IfCondition(camera),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'camera.launch.py'])),
            launch_arguments={'gazebo': 'True'}.items(),
        ),
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'urdf.launch.py'])),
            launch_arguments={'model': model, 'gazebo': 'true', 'robot_name': robot_name, 'angle': angle}.items(),
        ),
        IncludeLaunchDescription(
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([ros_gz_sim_dir, 'launch', 'gz_sim.launch.py'])),
            launch_arguments={'gz_args': ['-r -v4 ', world], 'on_exit_shutdown': 'True'}.items(),
        ),
        Node(
            package='ros_gz_sim',
            executable='create',
            output='screen',
            arguments=[
                '-topic', 'robot_description',
                '-name', robot_name,
                '-robot_namespace', namespace,
                '-x', pose['x'], '-y', pose['y'], '-z', pose['z'],
                '-R', pose['R'], '-P', pose['P'], '-Y', pose['Y']],
        ),
        Node(
            package='ros_gz_bridge',
            executable='bridge_node',
            name='ros_gz_bridge',
            namespace=namespace,
            output='screen',
            respawn=False,
            respawn_delay=2.0,
            parameters=[{'config_file': gz_params}],
            arguments=['--ros-args', '--log-level', 'info'],
        ),
    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')
    nova_gazebo_dir = FindPackageShare('nova_gazebo')
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [
        DeclareLaunchArgument(
            name='angle',
            default_value='15',
            description='Angle (in degrees) at which the camera is mounted',
        ),
        DeclareLaunchArgument(
            name='camera',
            default_value='True',
            description='',
        ),
        DeclareLaunchArgument(
            name='gz_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'gz_bridge.yaml']),
            description='Absolute path to ros_gz_bridge params file',
        ),
        DeclareLaunchArgument(
            name='controllers',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'controllers.yaml']),
            description='Absolute path to controllers params file',
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
            name='robot_name',
            default_value='Banksia',
            description='name of the robot',
        ),
        DeclareLaunchArgument(
            name='world',
            default_value=PathJoinSubstitution([nova_gazebo_dir, 'worlds', 'auto_cubes.sdf']),
            description='Full path to world model file to load',
        ),
        DeclareLaunchArgument(name='x', default_value='13.22', description='x_pose'),
        DeclareLaunchArgument(name='y', default_value='-7.50', description='y_pose'),
        DeclareLaunchArgument(name='z', default_value='0.5', description='z_pose'),
        DeclareLaunchArgument(name='R', default_value='0.0', description='roll'),
        DeclareLaunchArgument(name='P', default_value='0.0', description='pitch'),
        DeclareLaunchArgument(name='Y', default_value='0.0', description='yaw'),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )