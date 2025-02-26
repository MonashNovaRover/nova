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
AUTHOR:     Anthony Lew
CREATION:	06/02/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, AndSubstitution
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import IfCondition
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')

    gazebo = LaunchConfiguration('gazebo')
    debug_image = LaunchConfiguration('debug_image')
    depth_image = LaunchConfiguration('depth_image')
    depth_image_info = LaunchConfiguration('depth_image_info')
    namespace = LaunchConfiguration('namespace')
    params = LaunchConfiguration('params')
    rgb_image = LaunchConfiguration('rgb_image')
    use_debug = LaunchConfiguration('use_debug')
    yolo_model = LaunchConfiguration('yolo_model')

    return [
        Node(
            package='yolo_ros',
            executable='yolo_node',
            name='yolo_node',
            namespace=namespace,
            parameters=[{'model': yolo_model}, params],
            remappings=[('image_raw', rgb_image)],
            condition=IfCondition(gazebo)
        ),
        Node(
            package='yolo_ros',
            executable='debug_node',
            name='debug_node',
            namespace=namespace,
            parameters=[params],
            remappings=[('image_raw', rgb_image), 
                        ('dbg_image', debug_image)],
            condition=IfCondition(AndSubstitution(use_debug, gazebo)),
        ),
        Node(
            package='nova_utils',
            executable='cube_localiser.py',
            parameters=[{'sim': gazebo}, params],
            remappings=[('image_raw', depth_image), 
                        ('camera_info', depth_image_info),
                        ('detections', '/yolo/detections'),
                        ('cubes', '/yolo/cubes')],
            namespace=namespace,
        ),
    ]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='gazebo',
            default_value='True',
            description='Are we running this on gazebo?',
        ),
        DeclareLaunchArgument(
            name='debug_image',
            default_value='debug_image',
            description='Debug image topic used for yolo_ros',
        ),
        DeclareLaunchArgument(
            name='depth_image',
            default_value='/oak/stereo/image_raw',
            description='Depth image topic used for yolo_ros',
        ),
        DeclareLaunchArgument(
            name='depth_image_info',
            default_value='/oak/stereo/camera_info',
            description='Depth image info topic used for yolo_ros',
        ),
        DeclareLaunchArgument(
            name='namespace',
            default_value='yolo',
            description='Top-level namespace',
        ),
        DeclareLaunchArgument(
            name='params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'yolo.yaml']),
            description='Full path to the ROS2 parameters file to use for all launched nodes',
        ),
        DeclareLaunchArgument(
            name='rgb_image',
            default_value='/oak/rgb/image_raw',
            description='RGB image topic used for yolo_ros',
        ),
        DeclareLaunchArgument(
            name='use_debug',
            default_value='True',
            description='Enable yolo_ros debug node',
        ),
        DeclareLaunchArgument(
            name='yolo_model',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'resources', 'ARC_2025_sim', 'model.pt']),
            description='Absolute path to yolo weights file',
        ),
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
