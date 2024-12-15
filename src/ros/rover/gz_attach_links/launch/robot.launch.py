from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from ament_index_python.packages import get_package_share_directory
import os

def launch_setup(context, *args, **kwargs):
    gz_attach_links_dir = FindPackageShare('gz_attach_links')

    sliders = LaunchConfiguration('sliders').perform(context)
    robot = LaunchConfiguration('robot').perform(context)
    config_file = LaunchConfiguration('config_file')

    if robot == 'biglide':
        joints = ['A12', 'A21']
    else:
        joints = ['joint_A', 'joint_D']

    return [
        print(get_package_share_directory('qtbase')),
        AppendEnvironmentVariable(
            'QT_QPA_PLATFORM_PLUGIN_PATH', '/nix/store/k4riw99gl41nyvkr7bs4bmsmdpm96bj8-qtbase-5.15.14-bin/lib/qt-5.15.14/plugins/platforms'
        ),
        Node(
            package='ros_gz_sim',
            executable='create',
            output='screen',
            arguments=[
                '-name', robot,
                '-file', PathJoinSubstitution([gz_attach_links_dir, 'models', robot, f'{robot}.sdf']),
                '-z', '0.5'],
        ),
        Node(
            package='ros_gz_bridge',
            executable='bridge_node',
            name='ros_gz_bridge',
            output='screen',
            respawn=False,
            respawn_delay=2.0,
            parameters=[{'config_file': config_file}],
            arguments=['--ros-args', '--log-level', 'info'],
        ),
        Node(
            condition=IfCondition(sliders),
            package='slider_publisher',
            executable='slider_publisher',
            name='slider_publisher',
            arguments=[PathJoinSubstitution([gz_attach_links_dir, 'launch', 'effort_manual.yaml'])],
            output='screen'
        ),
    ]


def generate_launch_description():
    gz_attach_links_dir = FindPackageShare('gz_attach_links')

    declared_arguments = [
        DeclareLaunchArgument(
            name='sliders',
            default_value='True',
            description='Enable sliders for manual effort control.'
        ),
        DeclareLaunchArgument(
            name='robot',
            default_value='biglide',
            description='Specify the robot model to load.'
        ),
        DeclareLaunchArgument(
            name='config_file',
            default_value=PathJoinSubstitution([gz_attach_links_dir, 'launch', 'gz_bridge.yaml']), 
            description='Absolute path to ros_gz_bridge params file',
        ),
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )