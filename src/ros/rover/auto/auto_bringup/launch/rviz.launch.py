import os
from launch import LaunchDescription
from launch.conditions.unless_condition import IfCondition
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource

from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.actions import OpaqueFunction, ExecuteProcess

def launch_setup(context, *args, **kwargs):
    # package directories
    gazebo = LaunchConfiguration('gazebo').perform(context)
    rviz_params = LaunchConfiguration('rviz_params')
    model = LaunchConfiguration('model').perform(context)
    shortened_auto_mount = LaunchConfiguration('shortened_auto_mount').perform(context)
    robot_name = LaunchConfiguration('robot_name').perform(context)

    return [
        Node(
            package='rviz2',
            namespace='',
            executable='rviz2',
            name='rviz2',
            arguments=['-d', [rviz_params]]
        ),
        ExecuteProcess(
            cmd=['xacro', model,
                 f'gazebo:={gazebo}',
                 f'robot_name:={robot_name}',
                 'auto_mount:=True',
                 f'shortened_auto_mount:={shortened_auto_mount}',
                 '-o', os.path.expanduser("~/rviz.urdf")],
            output="screen"
        ),
    ]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')
    rover_description_dir = FindPackageShare('rover_description')

    launch_args = [
        DeclareLaunchArgument(
            name='gazebo',
            default_value='false',
            description='',
        ),
        DeclareLaunchArgument(
            name='rviz_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'rviz', 'everything.rviz']),
            description='Full path to the RViz config file to use',
        ),
        DeclareLaunchArgument(
            name='model',
            default_value=PathJoinSubstitution([rover_description_dir, 'banksia', 'urdf', 'rover.urdf.xacro']),
            description='Absolute path to robot urdf file',
        ),
        DeclareLaunchArgument(
            name='shortened_auto_mount',
            default_value='True',
            description='Whether to use the shortened auto mount model',
        ),
        DeclareLaunchArgument(
            name='robot_name',
            default_value='Banksia',
            description='name of the robot',
        ),
    ]

    return LaunchDescription( launch_args + [OpaqueFunction(function=launch_setup)])
