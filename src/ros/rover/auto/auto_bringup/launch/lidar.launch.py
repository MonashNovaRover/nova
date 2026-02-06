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
import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction, GroupAction, ExecuteProcess, RegisterEventHandler
from launch.event_handlers import OnProcessExit
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

# Similar to RTABMap, we want to delete previous maps when running FAST-LIVO2
# due to their large size and limited disk space on the device being run. 
def delete_fastlivo2_pcd(context, *args, **kwargs):
    dir_path = os.path.expanduser('~/.ros')

    raw_file_path = f"{dir_path}/all_raw_points.pcd"
    try:
        if os.path.exists(raw_file_path):
            os.remove(raw_file_path)
            print(f'Deleted all_raw_points.pcd at {raw_file_path}.')
        else:
            print(f'all_raw_points.pcd not found at {raw_file_path}. You might have to delete the file manually, wherever it is.')
    except Exception as e:
        print(f'Failed to delete file {raw_file_path}: {e}')

    downsampled_file_path = f"{dir_path}/all_downsampled_points.pcd"
    try:
        if os.path.exists(downsampled_file_path):
            os.remove(downsampled_file_path)
            print(f'Deleted all_downsampled_points.pcd at {downsampled_file_path}.')
        else:
            print(f'all_downsampled_points.pcd not found at {downsampled_file_path}. You might have to delete the file manually, wherever it is.')
    except Exception as e:
        print(f'Failed to delete file {downsampled_file_path}: {e}')

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')

    blackboard_params = LaunchConfiguration('blackboard_params')
    fastlivo2 = LaunchConfiguration('fastlivo2')
    fastlivo2_params = LaunchConfiguration('fastlivo2_params')
    lidar_config = LaunchConfiguration('lidar_config').perform(context)
    lidar_params = LaunchConfiguration('lidar_params')
    sim = LaunchConfiguration('sim')

    fastlivo2_node = GroupAction([
        Node(
            package='fast_livo',
            executable='fastlivo_mapping',
            name='fastlivo2',
            output='screen',
            parameters=[fastlivo2_params, {'use_sim_time': sim}],
        ),
    ])

    wait_for_parameter_blackboard = Node(
        package='nova_utils',
        executable='node_waiter.py',
        name='fastlivo2_wait_for_parameter_blackboard',
        output='screen',
        parameters=[
            {'nodes': ['parameter_blackboard']}
        ],
    )

    return [
        Node(
            condition=UnlessCondition(sim),
            package='livox_ros_driver2',
            executable='livox_ros_driver2_node',
            name='livox_lidar_publisher',
            output='screen',
            parameters=[lidar_params, {'user_config_path': lidar_config, 'use_sim_time': sim}],
        ),
        GroupAction(
            condition=IfCondition(fastlivo2),
            actions=[
                OpaqueFunction(function=delete_fastlivo2_pcd),
                Node(
                    package='demo_nodes_cpp',
                    executable='parameter_blackboard',
                    name='parameter_blackboard',
                    parameters=[blackboard_params, {'use_sim_time': sim}],
                    output='screen'
                ),
                # Node(
                #     package='tf2_ros',
                #     executable='static_transform_publisher',
                #     namespace='static_transform_publisher',
                #     name='odom_to_camera_init',
                #     output='screen',
                #     parameters=[{'use_sim_time': sim}],
                #     arguments=['0.541', '0.010', '0.950', '0.005', '0.698', '0.003', 'odom', 'camera_init'],
                # ),
                wait_for_parameter_blackboard,
                RegisterEventHandler(
                    event_handler=OnProcessExit(
                        target_action=wait_for_parameter_blackboard,
                        on_exit=fastlivo2_node,
                    ),
                ),
            ],
        ),
    ]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='blackboard_params',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','blackboard.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='fastlivo2',
            default_value='True',
            description='Use FAST-LIVO2?',
        ),
        DeclareLaunchArgument(
            name='fastlivo2_params',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','fastlivo2.yaml']),
            description='',
        ),
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
        DeclareLaunchArgument(
            name='sim',
            default_value='False',
            description='Use /clock instead of system clock?',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
