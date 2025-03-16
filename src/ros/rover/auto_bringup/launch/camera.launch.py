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
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import  Node, ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    depthai_dir = FindPackageShare('depthai_ros_driver')

    ar = LaunchConfiguration('ar')
    ar_params = LaunchConfiguration('ar_params')
    camera_model = LaunchConfiguration('camera_model')
    gazebo = LaunchConfiguration('gazebo')
    name = LaunchConfiguration('name').perform(context)
    oak_params = LaunchConfiguration('oak_params')
    parent_frame = LaunchConfiguration('parent_frame')
    pointclouds = LaunchConfiguration('pointclouds')
    rectify_image = LaunchConfiguration('rectify_image')

    return [
        IncludeLaunchDescription(
            condition=UnlessCondition(gazebo),
            launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([depthai_dir, 'launch', 'camera.launch.py'])),
            launch_arguments={'name': name,
                              'parent_frame': parent_frame,
                              'params_file': oak_params,
                              'camera_model': camera_model,
                              'rectify_rgb': 'false',
                              }.items()
        ),
        ComposableNodeContainer(
            name=f'{name}_image_proc_container',
            namespace='',
            package='rclcpp_components',
            executable='component_container',
            composable_node_descriptions=[
                ComposableNode(
                    condition=IfCondition(rectify_image),
                    package='image_proc',
                    plugin='image_proc::RectifyNode',
                    name='rectify_color_node',
                    remappings=[
                        ('image', f'{name}/rgb/image_raw'),
                        ('camera_info', f'{name}/rgb/camera_info'),
                        ('image_rect', f'{name}/rgb/image_rect')],
                ),
                ComposableNode(
                    condition=IfCondition(pointclouds),
                    package='rtabmap_util',
                    plugin='rtabmap_util::PointCloudXYZ',
                    name='point_cloud_xyz',
                    parameters=[{'decimation': 2,
                                'max_depth': 10.0,
                                'voxel_size': 0.1}],
                    remappings=[('depth/image', f'{name}/stereo/image_raw'),
                                ('depth/camera_info', f'{name}/stereo/camera_info'),
                                ('cloud', f'{name}/depth/points')],
                ),
            ],
        ),
        Node(
            condition=IfCondition(ar),
            package='aruco_opencv',
            executable='aruco_tracker_autostart',
            arguments=['--ros-args', '--params-file', ar_params],
        ),
        Node(
            package='imu_transformer',
            executable='imu_transformer_node',
            name='imu_transformer',
            remappings=[('/imu_in', '/oak/imu/data'),
                        ('/imu_out', '/oak/imu/transformed')],
            parameters=[{'target_frame': 'oak'}]
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
            name='camera_model',
            default_value='OAK-D-LR',
            description='',
        ),
        DeclareLaunchArgument(
            name='gazebo',
            default_value='False',
            description='',
        ),
        DeclareLaunchArgument(
            name='name',
            default_value='oak',
            description='',
        ),
        DeclareLaunchArgument(
            name='oak_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'oak.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='parent_frame',
            default_value='camera_link',
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
