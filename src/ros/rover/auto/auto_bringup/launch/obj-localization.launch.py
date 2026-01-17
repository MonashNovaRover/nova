'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Execute this code to run cube detection on sim
    Launches obj-localization.launch.py with specified args
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODES:
  - object_localiser
  - debug_node
TOPICS:
  INPUTS:
    - /oak/nn/spatial_detections [vision_msgs/Detection3DArray]
                                  (Primary input for localisation)
    - /oak/nn/detections         [vision_msgs/Detection2DArray] 
                                  (Used by debug_node for bounding boxes)
    - /oak/rgb/image_raw         [sensor_msgs/Image]
                                  (Used by debug_node for visualization)
  OUTPUTS:
    - /yolo/objects              [visualization_msgs/MarkerArray]
    - /yolo/debug_image          [sensor_msgs/Image]
    - /tf                        [geometry_msgs/TransformStamped]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_bringup
AUTHOR:     Chetan Edupalli
CREATION:	17/01/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import IfCondition
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    # Directories
    auto_bringup_dir = FindPackageShare('auto_bringup')

    # Configuration Variables
    namespace = LaunchConfiguration('namespace')
    yolo_params = LaunchConfiguration('yolo_params')
    
    # Flags
    use_debug = LaunchConfiguration('use_debug')
    
    # Topics
    rgb_image = LaunchConfiguration('rgb_image')
    detections_2d = LaunchConfiguration('detections_2d')
    detections_3d = LaunchConfiguration('detections_3d')
    debug_image_topic = LaunchConfiguration('debug_image_topic')

    # using OAK's internal spatial processing.
    using_oak = 'True'
    using_3d = 'True' 

    nodes = [
        # Object Localiser
        # Configured to listen to 3D spatial detections directly
        Node(
            package='nova_object_localisation',
            executable='object_localiser',
            name='object_localiser',
            namespace=namespace,
            parameters=[
                yolo_params,
                {
                    'using_oak': True,
                    'using_3d': True, # Enables processing of Detection3DArray
                    'detection_topic': detections_3d,
                }
            ],
            # Remappings aren't strictly necessary if 'detection_topic' param is set, 
            # but good for explicit dependency tracking
            remappings=[
                ('detections', detections_3d) 
            ]
        ),

        # Debug Node
        # Visualizes 2D bounding boxes on the RGB image.
        # Note: Debug node requires 2D detections (/oak/nn/detections), 
        # not the spatial 3D topic.
        Node(
            condition=IfCondition(use_debug),
            package='nova_object_localisation',
            executable='debug_node',
            name='debug_node',
            namespace=namespace,
            parameters=[yolo_params],
            remappings=[
                ('image_raw', rgb_image), 
                ('dbg_image', debug_image_topic),
                ('detections', detections_2d)
            ],
        ),
    ]

    return nodes

def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='namespace',
            default_value='',
            description='Top-level namespace',
        ),
        DeclareLaunchArgument(
            name='yolo_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'yolo.yaml']),
            description='Full path to the ROS2 parameters file',
        ),
        DeclareLaunchArgument(
            name='use_debug',
            default_value='True',
            description='Enable the debug image publisher',
        ),
        DeclareLaunchArgument(
            name='rgb_image',
            default_value='/oak/rgb/image_raw',
            description='RGB image topic used for debug visualization',
        ),
        DeclareLaunchArgument(
            name='detections_2d',
            default_value='/oak/nn/detections',
            description='2D detection topic used for debug visualization (vision_msgs/Detection2DArray)',
        ),
        DeclareLaunchArgument(
            name='detections_3d',
            default_value='/oak/nn/spatial_detections',
            description='3D spatial detection topic used for localisation (vision_msgs/Detection3DArray)',
        ),
        DeclareLaunchArgument(
            name='debug_image_topic',
            default_value='/yolo/debug_image',
            description='Output topic for the debug image with bounding boxes',
        ),
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )