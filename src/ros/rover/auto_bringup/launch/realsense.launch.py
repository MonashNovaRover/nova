'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Used for launching the Realsense D455 Camera
Also known as the Backside Observation and Optical Tracking for
Intelligent Exploration Camera.
AKA the Bootie Cam
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  -
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
CREATION:	13/11/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import  ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):

    realsense_params = LaunchConfiguration('realsense_params').perform(context)

    parent_frame = LaunchConfiguration('parent_frame')

    remappings = [
        ('/camera/realsense2_camera_node/color/image_raw','/bootie/rgb/image_raw'),
        ('/camera/realsense2_camera_node/color/camera_info','/bootie/rgb/camera_info'),
        ('/camera/realsense2_camera_node/depth/image_rect_raw','/bootie/depth/image_raw'),
        ('/camera/realsense2_camera_node/depth/camera_info','/bootie/depth/camera_info')
    ]

    return [
        ComposableNodeContainer(
            name='realsense_image_proc_container',
            namespace='booty',
            package='rclcpp_components',
            executable='component_container',
            composable_node_descriptions=[
                ComposableNode(
                    name='realsense2_camera_node',
                    package='realsense2_camera',
                    plugin='realsense2_camera::RealSenseNodeFactory',
                    parameters=[realsense_params],
                    remappings=remappings
                )
            ],
        ),

    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='realsense_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'realsense.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='parent_frame',
            default_value='booty_camera_link',
            description='',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
