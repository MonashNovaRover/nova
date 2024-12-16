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
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.conditions import IfCondition, LaunchConfigurationNotEquals
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import LoadComposableNodes, Node
from launch_ros.descriptions import ComposableNode
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = get_package_share_directory('auto_bringup')
    depthai_dir = get_package_share_directory('depthai_ros_driver')

    camera_model = LaunchConfiguration('camera_model').perform(context)
    name = LaunchConfiguration('name').perform(context)
    params_file = LaunchConfiguration('params_file')
    parent_frame = LaunchConfiguration('parent_frame').perform(context)
    use_camera = LaunchConfiguration('use_camera')

    return [
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([depthai_dir, 'launch', 'camera.launch.py'])),
            condition=IfCondition(use_camera),
            launch_arguments={'name': name,
                              'parent_frame': parent_frame,
                              'params_file': params_file,
                              'camera_model': camera_model}.items()
        ),
        LoadComposableNodes(
            condition=IfCondition(LaunchConfiguration('rectify_rgb')),
            target_container=name+'_container',
            composable_node_descriptions=[
                ComposableNode(
                    package='depth_image_proc',
                    plugin='depth_image_proc::PointCloudXyzNode',
                    name='point_cloud_xyz',
                    remappings=[('image_rect', f'{name}/stereo/image_filtered'),
                                ('camera_info', f'{name}/stereo/camera_info'),
                                ('points', f'{name}/points')
                                ]),
        ]),
        Node(
            condition=IfCondition(LaunchConfiguration('use_camera')),
            package='nova_pointcloud_filter',
            executable='depth_filter',
            name='depth_filter',
            remappings=[('/depth/image', f'{name}/stereo/image_raw'),
                        ('/depth/image_filtered', f'{name}/stereo/image_filtered'),
                        ],
            parameters=[{'t_filter': 0, 'r_filter': 0, 'b_filter': 75, 'l_filter': 0, }] 
        ),
        Node(
            condition=IfCondition(LaunchConfiguration('ar_tag')),
            package='aruco_opencv',
            executable='aruco_tracker_autostart',
            arguments=['--ros-args', '--params-file', PathJoinSubstitution([auto_bringup_dir, 'params', 'aruco_tracker.yaml'])],
        ),
        Node(
            package='imu_transformer',
            executable='imu_transformer_node',
            name='imu_transformer',
            remappings=[('/imu_in', '/oak/imu/data'),
                        ('/imu_out', '/oak/imu/transformed')
                       ],
            parameters=[{'target_frame':'oak'}]
        ),
    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')
    declared_arguments = [
        DeclareLaunchArgument('name', default_value='oak'),
        DeclareLaunchArgument('parent_frame', default_value='camera_link'),
        DeclareLaunchArgument('camera_model', default_value='OAK-D-LR'),
        DeclareLaunchArgument('params_file', default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'depthai_oakd_rgbd.yaml'])),
        DeclareLaunchArgument('rectify_rgb', default_value='True'),
        DeclareLaunchArgument('rtabmap_pointcloud', default_value='True'),
        DeclareLaunchArgument('ar_tag', default_value='True'),
        DeclareLaunchArgument('use_camera', default_value='True'),
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )