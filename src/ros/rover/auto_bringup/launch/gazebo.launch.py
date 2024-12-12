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
import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, AppendEnvironmentVariable, OpaqueFunction, SetEnvironmentVariable
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch.conditions import UnlessCondition, IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')
    nova_gazebo_dir = FindPackageShare('nova_gazebo')
    ros_gz_sim_dir = FindPackageShare('ros_gz_sim')

    config_file = LaunchConfiguration('config_file')
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
            PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'control.launch.py'])),
            launch_arguments={'gazebo': 'True'}.items(),
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'urdf.launch.py'])),
            launch_arguments={'model': model, 'gazebo': 'true', 'robot_name': robot_name}.items(),
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([ros_gz_sim_dir, 'launch', 'gz_sim.launch.py'])),
<<<<<<< HEAD
            launch_arguments={'gz_args': ['-r -v4 ', world], 'on_exit_shutdown': 'True'}.items(),
=======
            launch_arguments={'gz_args': ['-r -s -v4 ', world], 'on_exit_shutdown': 'True'}.items(),
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([ros_gz_sim_dir, 'launch', 'gz_sim.launch.py'])),
            launch_arguments={'gz_args': '-g -v4 '}.items(),
            condition=UnlessCondition(headless),
>>>>>>> a1ab665 (cleaned up launch files)
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
                '-R', pose['R'], '-P', pose['P'], '-Y', pose['Y'],
        ]),
        Node(
            package='ros_gz_bridge',
            executable='bridge_node',
            name='ros_gz_bridge',
            namespace=namespace,
            output='screen',
            respawn=False,
            respawn_delay=2.0,
            parameters=[{'config_file': config_file}],
            arguments=['--ros-args', '--log-level', 'info'],
        ),
    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')
    nova_gazebo_dir = FindPackageShare('nova_gazebo')
    rover_description_dir = FindPackageShare('new_rover_description')

    declared_arguments = [
        DeclareLaunchArgument(
            name='config_file',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'gz_bridge.yaml']), 
            description='Absolute path to ros_gz_bridge params file',
        ),
        DeclareLaunchArgument(
            name='launch_robot_description',
            default_value='True',
            description='Should gazebo launch its own robot description, or is one already running?',
        ),
        DeclareLaunchArgument(
            name='model', 
            default_value=PathJoinSubstitution([rover_description_dir, 'urdf', 'rover.urdf.xacro']), 
            description='Absolute path to robot urdf file',
        ),
        DeclareLaunchArgument(
            name='namespace',
            default_value='',
            description='Top-level namespace',
        ),
        DeclareLaunchArgument(
            name='robot_name',
            default_value='Rover7',
            description='name of the robot',
        ),
        DeclareLaunchArgument(
            name='world',
            default_value=PathJoinSubstitution([nova_gazebo_dir, 'worlds', 'auto.sdf']),
            description='Full path to world model file to load',
        ),
        DeclareLaunchArgument(name='x', default_value='0.0', description='x_pose'),
        DeclareLaunchArgument(name='y', default_value='0.0', description='y_pose'),
        DeclareLaunchArgument(name='z', default_value='0.45', description='z_pose'),
        DeclareLaunchArgument(name='R', default_value='0.00', description='roll'),
        DeclareLaunchArgument(name='P', default_value='0.00', description='pitch'),
        DeclareLaunchArgument(name='Y', default_value='0.00', description='yaw'),
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )