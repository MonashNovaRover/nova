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
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.conditions import IfCondition, UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, AndSubstitution, NotSubstitution
from launch_ros.actions import  Node, ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    cam_name = LaunchConfiguration('cam_name').perform(context)
    rs_params = LaunchConfiguration('rs_params')
    ar = LaunchConfiguration('ar')
    ar_params = LaunchConfiguration('ar_params')
    log_level = LaunchConfiguration('log_level')

    return [
        Node(
            package='realsense2_camera',
            name=cam_name,
            namespace='',
            executable='realsense2_camera_node',
            parameters=[rs_params],
            output='screen',
            arguments=['--ros-args', '--log-level', log_level],
            emulate_tty=True,
        ),
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            name='RS_D415_to_oak_link_static_tf_publisher',
            arguments=["0", "0", "0", "0", "0", "0", "RS_D415_link", "oak_link"],
            output='screen',
        ),
        Node(
            condition=IfCondition(ar),
            package='aruco_opencv',
            executable='aruco_tracker_autostart',
            parameters=[ar_params, {'cam_base_topic': f'/{cam_name}/color/image_raw'}],
            arguments=['--ros-args', '--log-level', log_level],
        ),
    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='cam_name',
            default_value='RS_D415',
            description='Name of camera',
        ),
        DeclareLaunchArgument(
            name='rs_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'rs_d415.yaml']),
            description='Path to realsense params file',
        ),
        DeclareLaunchArgument(
            name='ar',
            default_value='False',
            description='Detect AR tags?',
        ),
        DeclareLaunchArgument(
            name='ar_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'aruco_tracker.yaml']),
            description='Path to aruco tracker params file',
        ),
        DeclareLaunchArgument(
            name='log_level',
            default_value='info',
            description='',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
