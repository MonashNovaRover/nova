'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code to start usb cameras and
    associated utils.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - usb_cam_node_exe
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	18/02/2026
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
    ar = LaunchConfiguration('ar')
    ar_params = LaunchConfiguration('ar_params')
    rectify_image = LaunchConfiguration('rectify_image')

    ar_cam_topic = ''

    return [
        ComposableNodeContainer(
            condition=IfCondition(front),
            name=f'{front_name}_container',
            package='rclcpp_components',
            namespace='',
            executable='component_container',
            composable_node_descriptions=[
                ComposableNode(
                    condition=UnlessCondition(gazebo),
                    package='depthai_ros_driver',
                    plugin='depthai_ros_driver::Camera',
                    name=front_name,
                    parameters=[front_params],
                ),
                ComposableNode(
                    condition=IfCondition(rectify_image),
                    package='image_proc',
                    plugin='image_proc::RectifyNode',
                    name=f'{front_name}_rectify_color_node',
                    remappings=[
                        ('image', f'{front_name}/image_raw'),
                        ('camera_info', f'{front_name}/camera_info'),
                        ('image_rect', f'{front_name}/image_rect')],
                ),
            ],
        ),
        Node(
            condition=IfCondition(ar),
            package='aruco_opencv',
            executable='aruco_tracker_autostart',
            parameters=[
                ar_params,
                {"cam_base_topic": ar_cam_topic}
            ],
        ),
    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='ar',
            default_value='False',
            description='',
        ),
        DeclareLaunchArgument(
            name='ar_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'aruco_tracker.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='ar_cam_topic',
            default_value='',
            description='Camera topic for detecting AR tags',
        ),
        DeclareLaunchArgument(
            name='back',
            default_value='True',
            description='',
        ),
        DeclareLaunchArgument(
            name='back_name',
            default_value='bootie',
            description='',
        ),
        DeclareLaunchArgument(
            name='back_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'bootie.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='camera_model',
            default_value='OAK-D-LR',
            description='',
        ),
        DeclareLaunchArgument(
            name='front',
            default_value='True',
            description='',
        ),
        DeclareLaunchArgument(
            name='front_name',
            default_value='oak',
            description='',
        ),
        DeclareLaunchArgument(
            name='front_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'oak.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='gazebo',
            default_value='False',
            description='',
        ),
        DeclareLaunchArgument(
            name='imu',
            default_value='True',
            description='',
        ),
        DeclareLaunchArgument(
            name='mag',
            default_value='False',
            description='',
        ),
        DeclareLaunchArgument(
            name='pointclouds',
            default_value='True',
            description='',
        ),
        DeclareLaunchArgument(
            name='rectify_image',
            default_value='True',
            description='',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
