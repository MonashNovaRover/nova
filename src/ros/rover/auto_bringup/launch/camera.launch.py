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
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution, AndSubstitution, NotSubstitution
from launch_ros.actions import  Node, ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    ar = LaunchConfiguration('ar')
    ar_params = LaunchConfiguration('ar_params')
    back = LaunchConfiguration('back')
    back_name = LaunchConfiguration('back_name').perform(context)
    front = LaunchConfiguration('front')
    front_name = LaunchConfiguration('front_name').perform(context)
    gazebo = LaunchConfiguration('gazebo')
    imu = LaunchConfiguration('imu')
    oak_params = LaunchConfiguration('oak_params')
    bootie_params = LaunchConfiguration('bootie_params')
    pointcloud = LaunchConfiguration('pointcloud')
    rectify_image = LaunchConfiguration('rectify_image')

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
                    parameters=[oak_params],
                ),
                ComposableNode(
                    condition=IfCondition(rectify_image),
                    package='image_proc',
                    plugin='image_proc::RectifyNode',
                    name=f'{front_name}_rectify_color_node',
                    remappings=[
                        ('image', f'{front_name}/rgb/image_raw'),
                        ('camera_info', f'{front_name}/rgb/camera_info'),
                        ('image_rect', f'{front_name}/rgb/image_rect')],
                ),
                ComposableNode(
                    condition=IfCondition(pointcloud),
                    package='rtabmap_util',
                    plugin='rtabmap_util::PointCloudXYZ',
                    name=f'{front_name}_point_cloud_xyz',
                    parameters=[{'decimation': 2,
                                 'max_depth': 10.0,
                                 'voxel_size': 0.1}],
                    remappings=[('depth/image', f'{front_name}/stereo/image_raw'),
                                ('depth/camera_info', f'{front_name}/stereo/camera_info'),
                                ('cloud', f'{front_name}/points')],
                ),
                ComposableNode(
                    condition=IfCondition(AndSubstitution(imu, NotSubstitution(gazebo))),
                    package='imu_transformer',
                    plugin='imu_transformer::ImuTransformer',
                    name=f'{front_name}_imu_transformer_node',
                    remappings=[('/imu_in', f'/{front_name}/imu/data'),
                                ('/imu_out', f'/{front_name}/imu/transformed')],
                    parameters=[{'target_frame': f'{front_name}_imu_frame'}]
                ),
            ],
        ),
        ComposableNodeContainer(
            condition=IfCondition(back),
            name=f'{back_name}_container',
            package='rclcpp_components',
            namespace='',
            executable='component_container',
            composable_node_descriptions=[
                ComposableNode(
                    condition=UnlessCondition(gazebo),
                    package='depthai_ros_driver',
                    plugin='depthai_ros_driver::Camera',
                    name=back_name,
                    parameters=[bootie_params],
                ),
                ComposableNode(
                    condition=IfCondition(rectify_image),
                    package='image_proc',
                    plugin='image_proc::RectifyNode',
                    name=f'{back_name}_rectify_color_node',
                    remappings=[('image', f'{back_name}/rgb/image_raw'),
                                ('camera_info', f'{back_name}/rgb/camera_info'),
                                ('image_rect', f'{back_name}/rgb/image_rect')],
                ),
                ComposableNode(
                    condition=IfCondition(pointcloud),
                    package='rtabmap_util',
                    plugin='rtabmap_util::PointCloudXYZ',
                    name=f'{back_name}_point_cloud_xyz',
                    parameters=[{'decimation': 2,
                                 'max_depth': 10.0,
                                 'min_depth': 1.1,
                                 'voxel_size': 0.1}],
                    remappings=[('depth/image', f'{back_name}/stereo/image_raw'),
                                ('depth/camera_info', f'{back_name}/stereo/camera_info'),
                                ('cloud', f'{back_name}/points')],
                ),
                ComposableNode(
                    condition=IfCondition(AndSubstitution(imu, NotSubstitution(gazebo))),
                    package='imu_filter_madgwick',
                    plugin='ImuFilterMadgwickRos',
                    name=f'{back_name}_imu_filter_node',
                    remappings=[('imu/data_raw', f'/{back_name}/imu/data'),
                                ('imu/mag', f'/{back_name}/imu/mag'),
                                ('imu/data', f'/{back_name}/imu/fused')],
                    parameters=[{'use_mag': True,
                                 'world_frame': 'enu',
                                 'fixed_frame': 'bootie_link',
                                 'publish_tf': True,
                                 'reverse_accel': False,
                                 'mag_bias_x': 0.0,
                                 'mag_bias_y': 0.0,
                                 'mag_bias_z': 0.0}]
                ),
                ComposableNode(
                    condition=IfCondition(AndSubstitution(imu, NotSubstitution(gazebo))),
                    package='imu_transformer',
                    plugin='imu_transformer::ImuTransformer',
                    name=f'{back_name}_imu_transformer_node',
                    remappings=[('/imu_in', f'/{back_name}/imu/fused'),
                                ('/imu_out', f'/{back_name}/imu/transformed')],
                    parameters=[{'target_frame': f'{back_name}_imu_frame'}]
                ),
            ]
        ),
        Node(
            condition=IfCondition(ar),
            package='aruco_opencv',
            executable='aruco_tracker_autostart',
            arguments=['--ros-args', '--params-file', ar_params],
        ),
    ]


def generate_launch_description():
    auto_bringup_dir = FindPackageShare('auto_bringup')

    declared_arguments = [
        DeclareLaunchArgument(
            name='ar',
            default_value='True',
            description='',
        ),
        DeclareLaunchArgument(
            name='ar_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'aruco_tracker.yaml']),
            description='',
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
            name='imu',
            default_value='True',
            description='',
        ),
        DeclareLaunchArgument(
            name='oak_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'oak.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='bootie_params',
            default_value=PathJoinSubstitution([auto_bringup_dir, 'params', 'bootie.yaml']),
            description='',
        ),
        DeclareLaunchArgument(
            name='pointcloud',
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