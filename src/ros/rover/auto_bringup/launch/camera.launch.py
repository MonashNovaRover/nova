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
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import  Node, ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
from launch_ros.substitutions import FindPackageShare
from launch.substitutions import EqualsSubstitution

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = get_package_share_directory('auto_bringup')
    depthai_dir = get_package_share_directory('depthai_ros_driver')

    camera_model = LaunchConfiguration('camera_model').perform(context)
    name = LaunchConfiguration('name').perform(context)
    params_file = LaunchConfiguration('params_file')
    parent_frame = LaunchConfiguration('parent_frame').perform(context)
    use_filter = LaunchConfiguration('use_filter').perform(context)

    return [
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([depthai_dir, 'launch', 'camera.launch.py'])),
            condition=IfCondition(
                EqualsSubstitution(LaunchConfiguration('gazebo'), "False")
            ),
            launch_arguments={'name': name,
                              'parent_frame': parent_frame,
                              'params_file': params_file,
                              'camera_model': camera_model,
                              'rectify_rgb': 'false',
                              }.items()
        ),
        Node(
            condition=IfCondition(use_filter),
            package='nova_pointcloud_filter',
            executable='depth_filter',
            name='depth_filter',
            remappings=[('/depth/image', f'{name}/stereo/image_raw'),
                        ('/depth/image_filtered', f'{name}/stereo/image_filtered')],
            parameters=[{'t_filter': 0, 'r_filter': 0, 'b_filter': 75, 'l_filter': 0}]
        ),
        ComposableNodeContainer(
            name=f"{name}_image_proc_container",
            namespace='',
            package="rclcpp_components",
            executable="component_container",
            composable_node_descriptions=[
                ComposableNode(
                    condition=IfCondition(LaunchConfiguration('rectify_image')),
                    package='image_proc',
                    plugin='image_proc::RectifyNode',
                    name='rectify_color_node',
                    remappings=[
                        ('image', f'{name}/rgb/image_raw'),
                        ('camera_info', f'{name}/rgb/camera_info'),
                        ('image_rect', f'{name}/rgb/image_rect')
                    ]),
                ComposableNode(
                    condition=IfCondition(LaunchConfiguration('pointclouds')),
                    package='depth_image_proc',
                    plugin='depth_image_proc::PointCloudXyzNode',
                    name='point_cloud_xyz',
                    remappings=[('image_rect', f'{name}/stereo/image_raw'),
                                ('camera_info', f'{name}/stereo/camera_info'),
                                ('points', f'{name}/depth/points')]
                ),
            ]
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
                        ('/imu_out', '/oak/imu/transformed')],
            parameters=[{'target_frame': 'oak'}]
        ),
    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')
    declared_arguments = [
        DeclareLaunchArgument('name', default_value='oak'),
        DeclareLaunchArgument('parent_frame', default_value='camera_link'),
        DeclareLaunchArgument('camera_model', default_value='OAK-D-LR'),
        DeclareLaunchArgument('params_file', default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'depthai_oakd_rgbd.yaml'])),
        DeclareLaunchArgument('ar_tag', default_value='True'),
        DeclareLaunchArgument('use_filter', default_value='False'), # To Be Exoerimented so False for now
        DeclareLaunchArgument('gazebo', default_value='False'),
        DeclareLaunchArgument('pointclouds', default_value='True'),
        DeclareLaunchArgument('rectify_image', default_value='True')
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
