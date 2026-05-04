'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code on the rover to start all
   auto camera scripts.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  -
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	13/11/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from os.path import expanduser

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import IfElseSubstitution, LaunchConfiguration, PathJoinSubstitution, EnvironmentVariable
from launch_ros.actions import ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    comp = LaunchConfiguration('comp').perform(context).lower()

    # comp agnostic arguments
    cam_name = LaunchConfiguration('cam_name').perform(context)
    rs_params = LaunchConfiguration('rs_params')
    ar_params = LaunchConfiguration('ar_params')
    sim = LaunchConfiguration('sim')

    # comp defaults
    if comp == 'arch':
        ar = 'False'
    elif comp == 'urc':
        ar = 'True'
    else:
        raise ValueError('Invalid comp value')
    
    # comp defaults overrides
    if LaunchConfiguration('ar').perform(context) != '':
        ar = LaunchConfiguration('ar')

    return [
        ComposableNodeContainer(
            name='realsense_container',
            namespace='',
            package='rclcpp_components',
            executable='component_container',
            composable_node_descriptions=[
                ComposableNode(
                    condition=UnlessCondition(sim),
                    package='realsense2_camera',
                    plugin='realsense2_camera::RealSenseNodeFactory',
                    name='d415',
                    namespace='',
                    parameters=[rs_params, {'camera_name': cam_name}],
                ),
                ComposableNode(
                    condition=IfCondition(ar),
                    package='aruco_opencv',
                    plugin='aruco_opencv::ArucoTrackerAutostart',
                    name='aruco_tracker',
                    namespace='',
                    parameters=[ar_params,
                                {'cam_base_topic': f'{cam_name}/color/image_raw',
                                 'use_sim_time': sim}],
                ),
            ],
        ),
    ]


def generate_launch_description():
    local = LaunchConfiguration('local')

    auto_bringup_dir = IfElseSubstitution(local,
        PathJoinSubstitution([expanduser("~") + '/nova/src/ros/rover/auto/auto_bringup']),
        FindPackageShare('auto_bringup')
    )

    declared_arguments = [
        DeclareLaunchArgument(
            name='local',
            default_value='False',
            description='Whether to use local directories instead of the nix store.',
        ),
        DeclareLaunchArgument(
            name='comp',
            default_value=EnvironmentVariable('COMP', default_value='ARCh'),
            description='ARCh or URC',
        ),
        DeclareLaunchArgument(
            name='cam_name',
            default_value='d415',
            description='Name of camera',
        ),
        DeclareLaunchArgument(
            name='rs_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'd415.yaml']),
            description='Path to realsense params file',
        ),
        # arg with comp defaults
        DeclareLaunchArgument(
            name='ar',
            default_value='',
            description='Detect AR tags?',
        ),
        DeclareLaunchArgument(
            name='ar_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'aruco_tracker.yaml']),
            description='Path to aruco tracker params file',
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
