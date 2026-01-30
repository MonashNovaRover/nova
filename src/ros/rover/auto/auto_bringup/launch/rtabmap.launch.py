import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.descriptions import ComposableNode
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import ComposableNodeContainer

# This function does what --delete_db_on_start
# Because we are launching it as a ComposableNode, we can't pass arguments to the executable
# Hence we delete the db file here for it to not mess with us later
def delete_rtabmap_db(context, *args, **kwargs):
    file_path = os.path.expanduser('~/.ros/rtabmap.db')
    try:
        if os.path.exists(file_path):
            os.remove(file_path)
            print(f'Deleted rtabmap.db at {file_path}')
        else:
            print(f'rtabmap.db not found at {file_path}. You might have to delete the file manually, wherever it is')
    except Exception as e:
        print(f'Failed to delete file {file_path}: {e}')

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')

    gazebo = LaunchConfiguration('gazebo')
    front_name = LaunchConfiguration('front_name').perform(context)
    back_name = LaunchConfiguration('back_name').perform(context)
    x = LaunchConfiguration('x').perform(context)
    y = LaunchConfiguration('y').perform(context)
    z = LaunchConfiguration('z').perform(context)
    roll = LaunchConfiguration('roll').perform(context)
    pitch = LaunchConfiguration('pitch').perform(context)
    yaw = LaunchConfiguration('yaw').perform(context)
    camera = LaunchConfiguration('camera').perform(context)
    rtabmap_viz = LaunchConfiguration('rtabmap_viz').perform(context)
    rtabmap_params = LaunchConfiguration('rtabmap_params').perform(context)


    return [
        OpaqueFunction(function=delete_rtabmap_db),
        IncludeLaunchDescription(
            condition=IfCondition(camera),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'camera.launch.py'])),
            launch_arguments={
                'pointclouds':'False',
                'gazebo': gazebo,
            }.items()
        ),
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            name='base_to_lidar_tf',
            arguments=[
                '0', '0', '0.5',     # X, Y, Z
                '0', '0', '0',       # Roll, Pitch, Yaw
                'base_link',         # Parent frame
                'livox_frame'        # Child frame
            ],
        ),
        ComposableNodeContainer(
            name='rtabmap_mapping_container',
            namespace='',
            package='rclcpp_components',
            executable='component_container',
            composable_node_descriptions=[

                # 1. ICP Odometry (Tracks Movement using LiDAR)
                ComposableNode(
                    package='rtabmap_odom',
                    plugin='rtabmap_odom::ICPOdometry',
                    name='icp_odometry',
                    parameters=[rtabmap_params, {
                        'use_sim_time': gazebo,
                        'frame_id': 'base_link',
                        'odom_frame_id': 'odom',
                        'wait_for_transform': 0.2,
                        'expected_update_rate': 15.0,
                        'Icp/PointToPlane': 'true',
                        'subscribe_scan': 'false', 
                        'subscribe_scan_cloud': 'true',
                        'qos': 2, # Critical for Sim
                    }],
                    remappings=[
                        ('scan_cloud', '/livox/lidar'), 
                        ('odom', '/odometry/local'),    
                        ('imu', '/livox/imu'),          
                    ],
                ),

                # 2. SLAM Node (Directly Syncs RGB + LiDAR)
                ComposableNode(
                    package='rtabmap_slam',
                    plugin='rtabmap_slam::CoreWrapper',
                    name='rtabmap_slam',
                    parameters=[rtabmap_params, {
                        'use_sim_time': gazebo, 
                        'rtabmap_args': '--delete_db_on_start',
                        
                        'subscribe_rgb': False,
                        'subscribe_rgbd': False, #set to true to use depth images
                        'subscribe_scan_cloud': True,
                        
                        'subscribe_depth': False,  
                        'subscribe_stereo': False,  
                        'subscribe_scan': False,      

                        'approx_sync': True,
                        'approx_sync_max_interval': 0.5,
                        
                        'qos': 2,
                        'topic_queue_size': 30,
                        'sync_queue_size': 30,
                    }],
                    remappings=[
                        # Connect RGB Input to the CLEANED topic
                        # ('rgb/image',       '/oak/rgb/image_raw/clean'),
                        # ('rgb/camera_info', '/oak/rgb/camera_info'),
                        
                        # Connect Geometry Input DIRECTLY to LiDAR
                        ('scan_cloud',      '/livox/lidar'),
                        
                        ('odom', '/odometry/local'),    
                        ('gps/fix','/gps_rover/fix')
                    ],
                ),
            ],
        ),
        Node(
            package='image_transport',
            executable='republish',
            name='rgb_republish',
            arguments=['raw', 'raw'], # Input -> Output
            remappings=[
                ('in', '/oak/rgb/image_raw'),
                ('out', '/oak/rgb/image_raw/clean') # New clean topic
            ],
            output='screen'
        ),
        Node(
             package='rtabmap_util', executable='obstacles_detection', output='screen',
             parameters=[rtabmap_params],
             remappings=[
                 ('cloud','/livox/lidar'),
                 ('obstacles','/livox/lidar/obstacles'),
                 ('ground', '/livox/lidar/ground'),
             ],
         ),
        Node(
            condition=IfCondition(rtabmap_viz),
            package='rtabmap_viz',
            executable='rtabmap_viz',
            output='screen',
            parameters=[rtabmap_params, {'use_sim_time': True}],
            remappings=[
                ('scan_cloud','/livox/lidar'),
                ('rgb/image','/oak/rgb/image_raw'),
                ('rgb/camera_info','/oak/rgb/camera_info'),
                ('odom', '/odometry/local')
            ]
        ),
    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(name='front_name', default_value='oak'),
        DeclareLaunchArgument(name='back_name', default_value='bootie'),
        DeclareLaunchArgument(name='rtabmap_viz', default_value='False'),
        DeclareLaunchArgument(name='gazebo', default_value='False'),
        DeclareLaunchArgument(name='camera', default_value='False'),
        DeclareLaunchArgument(name='x', default_value='0.0'),
        DeclareLaunchArgument(name='y', default_value='0.0'),
        DeclareLaunchArgument(name='z', default_value='0.0'),
        DeclareLaunchArgument(name='roll', default_value='0.0'),
        DeclareLaunchArgument(name='pitch', default_value='0.0'),
        DeclareLaunchArgument(name='yaw', default_value='0.0'),
        DeclareLaunchArgument(
            name='rtabmap_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'rtabmap.yaml']),
            description='Params file for RTABMap Nodes',
        ),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
