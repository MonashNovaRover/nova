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
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, AndSubstitution, NotSubstitution
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import IfCondition, UnlessCondition
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')

    using_oak = LaunchConfiguration('using_oak')
    using_3d = LaunchConfiguration('using_3d')
    namespace = LaunchConfiguration('namespace')
    yolo_params = LaunchConfiguration('yolo_params')
    rgb_image = LaunchConfiguration('rgb_image')
    use_debug = LaunchConfiguration('use_debug')
    debug_image = LaunchConfiguration('debug_image')
    depth_image = LaunchConfiguration('depth_image')
    depth_camera_info = LaunchConfiguration('depth_camera_info')
    gazebo = (LaunchConfiguration('gazebo').perform(context).lower() == 'true')
    yolo_model = LaunchConfiguration('yolo_model') # for yolo_ros only
    detections = LaunchConfiguration('detections')
    detections_3d = LaunchConfiguration('detections_3d')

    if gazebo:
        yolo_params = PathJoinSubstitution([auto_bringup_dir, 'params', 'yolo_sim.yaml'])

    return [
        # yolo_ros nodes only run if using_oak is false
        # 3d mode is not supported for yolo_ros as the 3d yolo_ros code is copied into object_localiser anyway
        Node(
            condition=UnlessCondition(using_oak), 
            package='yolo_ros',
            executable='yolo_node',
            name='yolo_ros_node',
            namespace=namespace,
            parameters=[{'model': yolo_model}, yolo_params],
            remappings=[('image_raw', rgb_image),
                        ('detections', detections)],
        ),
        Node(
            condition=IfCondition(AndSubstitution(use_debug, NotSubstitution(using_oak))),
            package='yolo_ros',
            executable='debug_node',
            name='yolo_ros_debug_node',
            namespace=namespace,
            parameters=[yolo_params],
            remappings=[('image_raw', rgb_image), 
                        ('dbg_image', debug_image),
                        ('detections', detections)],
        ),
        Node(
            condition=IfCondition(AndSubstitution(use_debug, using_oak)),
            package='nova_object_localisation',
            executable='debug_node',
            name='yolo_ros_debug_node',
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
            parameters=[{'using_oak': using_oak, 'using_3d': using_3d}, yolo_params],
            namespace=namespace,
        ),
    ]

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='using_oak',
            default_value='True',
            description='Are we running this with the OAK camera?',
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
            default_value='/oak/rgb/image_raw',
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
            default_value=PathJoinSubstitution([auto_bringup_dir, 'resources', 'YOLO_ARCh_2025', 'best.pt']),
            description='Absolute path to yolo weights file for yolo_ros',
        ),
        DeclareLaunchArgument(
            name='depth_image',
            default_value='/oak/stereo/image_raw',
            description='Depth image topic used for yolo_ros',
        ),
        DeclareLaunchArgument(
            name='depth_camera_info',
            default_value='/oak/stereo/camera_info',
            description='Depth image info topic used for yolo_ros',
        ),
        DeclareLaunchArgument(
            name='detections',
            default_value='/oak/nn/detections',
            description='Output detection topic used for yolo_ros',
        ),
        DeclareLaunchArgument(
            name='detections_3d',
            default_value='/oak/nn/spatial_detections',
            description='Output 3d detection topic used for yolo_ros',
        ),
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )