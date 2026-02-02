import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare
from launch.conditions.unless_condition import IfCondition

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')
    use_rviz = LaunchConfiguration('rviz')
    rviz_params = LaunchConfiguration('rviz_params')
    fast_calib_params = LaunchConfiguration('fast_calib_params')

    return [
        Node(
            package='fast_calib',
            executable='fast_calib',
            name='mono_qr_pattern',
            output='screen',
            parameters=[fast_calib_params]
        ),
        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz2',
            arguments=['-d', [rviz_params]],
            condition=IfCondition(use_rviz)
        )
    ]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='rviz',
            default_value='true',
            description='Whether to start RViz'
        ),
        DeclareLaunchArgument(
            name='rviz_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'rviz', 'fast_calib.rviz']),
            description=''
        ),
        DeclareLaunchArgument(
            name='fast_calib_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'fast_calib.yaml']),
            description='Fast Calib params'
        )
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )