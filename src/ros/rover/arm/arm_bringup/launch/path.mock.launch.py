'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CREATION:	23/2/2025
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
    arm_bringup_dir = FindPackageShare('arm_bringup')
    nova_gazebo_dir = FindPackageShare('nova_gazebo')
    ros_gz_sim_dir = FindPackageShare('ros_gz_sim')

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
    controllers = LaunchConfiguration('controllers')
    arm = LaunchConfiguration('arm')
    old_arm = LaunchConfiguration('old_arm')

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
            PythonLaunchDescriptionSource(PathJoinSubstitution([arm_bringup_dir, 'launch', 'path.control.launch.py'])),
            launch_arguments={'controllers': controllers, 'model': model, 'gazebo': 'False', 'robot_name': robot_name, 'arm': arm, 'old_arm': old_arm, 'use_mock_hardware': 'True'}.items(),
        ),
        # I think this is already handled by control.launch.py
        # IncludeLaunchDescription(
        #     PythonLaunchDescriptionSource(PathJoinSubstitution([arm_bringup_dir, 'launch', 'urdf.launch.py'])),
        #     launch_arguments={'model': model, 'gazebo': 'false', 'robot_name': robot_name, 'arm': arm, 'use_mock_hardware': 'True'}.items(),
        # ),
    ]

def generate_launch_description():
    arm_bringup_dir = FindPackageShare('arm_bringup')
    nova_gazebo_dir = FindPackageShare('nova_gazebo')
    rover_description_dir = FindPackageShare('rover_description')

    declared_arguments = [
        DeclareLaunchArgument(
            name='controllers',
            default_value=PathJoinSubstitution([arm_bringup_dir, 'params', 'controllers.yaml']),
            description='Absolute path to controllers params file',
        ),
        DeclareLaunchArgument(
            name='launch_robot_description',
            default_value='True',
            description='Should gazebo launch its own robot description, or is one already running?',
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
            default_value=PathJoinSubstitution([nova_gazebo_dir, 'worlds', 'auto.sdf']),
            description='Full path to world model file to load',
        ),
        DeclareLaunchArgument(
            name='arm',
            default_value='false',
            description='whether to launch arm',
        ),
        DeclareLaunchArgument(
            name='old_arm',
            default_value='True',
            description='whether to launch the old arm (on new armware)',
        ),
        DeclareLaunchArgument(name='x', default_value='11.2123871', description='x_pose'),
        DeclareLaunchArgument(name='y', default_value='-10.1349831', description='y_pose'),
        DeclareLaunchArgument(name='z', default_value='0.5', description='z_pose'),
        DeclareLaunchArgument(name='R', default_value='0.0', description='roll'),
        DeclareLaunchArgument(name='P', default_value='0.0', description='pitch'),
        DeclareLaunchArgument(name='Y', default_value='2.5740044', description='yaw'),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
