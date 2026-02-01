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
from launch.actions import DeclareLaunchArgument, OpaqueFunction, GroupAction, ExecuteProcess, RegisterEventHandler
from launch.event_handlers import OnProcessExit
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import IfCondition, UnlessCondition
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')

    blackboard_params = LaunchConfiguration('blackboard_params')
    fast_livo2 = LaunchConfiguration('fast_livo2')
    fast_livo2_params = LaunchConfiguration('fast_livo2_params')
    lidar_config = LaunchConfiguration('lidar_config').perform(context)
    lidar_params = LaunchConfiguration('lidar_params')
    sim = LaunchConfiguration('sim')

    fast_livo2_node = GroupAction([
        Node(
            condition=IfCondition(fast_livo2),
            package='fast_livo',
            executable='fastlivo_mapping',
            name='fastlivo2',
            output='screen',
            parameters=[fast_livo2_params, {'use_sim_time': sim}],
            # remappings=[
            # ],
        ),
    ])

    wait_for_parameter_blackboard = ExecuteProcess(
        cmd=[
            'python3',
            PathJoinSubstitution([auto_bringup_dir, 'topic', 'wait_for_node.py']),
        ],
        name='wait_for_parameter_blackboard',
        output='screen',
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
        Node(
            condition=IfCondition(fast_livo2),
            package='demo_nodes_cpp',
            executable='parameter_blackboard',
            name='parameter_blackboard',
            parameters=[blackboard_params, {'use_sim_time': sim}],
            output='screen'
        ),
        wait_for_parameter_blackboard,
        RegisterEventHandler(
            OnProcessExit(
                target_action=wait_for_parameter_blackboard,
                on_exit=[fast_livo2_node],
            ),
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
            name='fast_livo2',
            default_value='True',
            description='Use FAST-LIVO2?',
        ),
        DeclareLaunchArgument(
            name='fast_livo2_params',
            default_value=PathJoinSubstitution([auto_bringup_dir,'params','fast_livo2.yaml']),
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
