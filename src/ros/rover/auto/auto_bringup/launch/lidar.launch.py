'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This launch file is responsible for LiDAR.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - livox_lidar_publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	15/01/2026
EDITED:     15/01/2026
EDITED BY:  Kabilan Velmurugan Sujatha, Bailey 
    Chessum, Victor Bartlinski
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    lidar_config = LaunchConfiguration('lidar_config').perform(context)
    lidar_params = LaunchConfiguration('lidar_params')

    return [
        Node(
            package='livox_ros_driver2',
            executable='livox_ros_driver2_node',
            name='livox_lidar_publisher',
            output='screen',
            parameters=[lidar_params, {'user_config_path': lidar_config}],
        ),
    ]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='lidar_config',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','lidar_config.json']),
            description='',
        ),
        DeclareLaunchArgument(
            name='lidar_params',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','lidar.yaml']),
            description='',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )