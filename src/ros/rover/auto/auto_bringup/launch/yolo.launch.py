'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code to run cube detection on sim
    Launches yolo.launch.py with specified args
A modified yolo launch file from yolo_ros package
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - yolo_node
  - debug_node
  - tracking_node
  - detect_3d_node
TOPICS:
  INPUTS:
    - /oak/rgb/image_rect
    - /oak/depth
    - /oak/camera_info
  OUTPUTS:
    - /yolo (many topics under this)
    - /yolo/detections_3d (important! used
                            for cube localisation)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
AUTHOR:     Anthony Lew, Chetan Edupalli
EDITED:	    04/03/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, AndSubstitution, NotSubstitution
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import IfCondition, UnlessCondition
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')
    use_vision_msgs_config = LaunchConfiguration('use_vision_msgs')
    use_debug_config = LaunchConfiguration('use_debug')
    use_vision_msgs_val = use_vision_msgs_config.perform(context).lower() == 'true'
    using_3d_val = LaunchConfiguration('using_3d').perform(context).lower() == 'true'
    namespace = LaunchConfiguration('namespace')
    yolo_params = LaunchConfiguration('yolo_params')
    rgb_image = LaunchConfiguration('rgb_image')
    debug_image = LaunchConfiguration('debug_image')
    gazebo = (LaunchConfiguration('gazebo').perform(context).lower() == 'true')
    yolo_model = LaunchConfiguration('yolo_model') 
    detections = LaunchConfiguration('detections')

    if gazebo:
        yolo_params = PathJoinSubstitution([auto_bringup_dir, 'params', 'yolo_sim.yaml'])

    return [
        Node(
            condition=UnlessCondition(use_vision_msgs_config),
            package='yolo_ros',
            executable='yolo_node',
            name='yolo_ros_node',
            namespace=namespace,
            parameters=[{'model': yolo_model}, yolo_params],
            remappings=[('image_raw', rgb_image),
                        ('detections', detections)],
        ),
        Node(
            condition=UnlessCondition(use_vision_msgs_config), 
            package='yolo_ros',
            executable='debug_node',
            name='yolo_ros_debug_node',
            namespace=namespace,
            parameters=[yolo_params],
            remappings=[('image_raw', rgb_image), 
                        ('debug_image', debug_image),
                        ('detections', detections)],
        ),
        Node(
            condition=IfCondition(use_vision_msgs_config),
            package='nova_object_localisation', 
            executable='debug_node',
            name='debug_node',
            namespace=namespace,
            parameters=[yolo_params],
            remappings=[('image_raw', rgb_image), 
                        ('dbg_image', debug_image),
                        ('detections', detections)],
        ),
        Node(
            package='nova_object_localisation',
            executable='object_localiser',
            name='object_localiser',
            parameters=[{
                'use_vision_msgs': use_vision_msgs_val, 
                'using_3d': using_3d_val, 
                'frame_id': LaunchConfiguration('frame_id')
            }, yolo_params],
            namespace=namespace,
        ),
    ]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='use_vision_msgs',
            default_value='True',
            description='use vision_msgs types for yolo_ros output',
        ),
        DeclareLaunchArgument(
            name='using_3d',
            default_value='False',
            description='Is 3D points already being handled by YOLO?',
        ),
        DeclareLaunchArgument(
            name='namespace',
            default_value='',
            description='Top-level namespace',
        ),
        DeclareLaunchArgument(
            name='yolo_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'yolo.yaml']),
            description='Full path to the ROS2 parameters file to use for all launched nodes',
        ),
        DeclareLaunchArgument(
            name='rgb_image',
            default_value='/camera/color/image_raw',
            description='RGB image topic used for yolo_ros',
        ),
        DeclareLaunchArgument(
            name='use_debug',
            default_value='True',
            description='Enable yolo_ros debug node',
        ),
        DeclareLaunchArgument(
            name='debug_image',
            default_value='/yolo/debug_image',
            description='Output debug image topic used for yolo_ros',
        ),
        DeclareLaunchArgument(
            name='gazebo',
            default_value='False',
            description='',
        ),
        DeclareLaunchArgument(
            name='yolo_model',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'resources', 'YOLO_URC_2025', 'yolo11s.pt']),
            description='Absolute path to yolo weights file for yolo_ros',
        ),
        DeclareLaunchArgument(
            name='depth_image',
            default_value='/camera/depth/image_raw',
            description='Depth image topic used for yolo_ros',
        ),
        DeclareLaunchArgument(
            name='depth_camera_info',
            default_value='/camera/depth/camera_info',
            description='Depth image info topic used for yolo_ros',
        ),
        DeclareLaunchArgument(
            name='detections',
            default_value='/detections_2d',
            description='Output detection topic used for yolo_ros',
        ),
        DeclareLaunchArgument(
            name='detections_3d',
            default_value='/detections_3d',
            description='Output 3d detection topic used for yolo_ros',
        ),
        DeclareLaunchArgument(
            name='frame_id', 
            default_value='camera_color_optical_frame',
            description='frame_id to use for 3D detections'
        ),
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )