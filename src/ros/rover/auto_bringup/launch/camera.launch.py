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
    # depthai_dir = FindPackageShare('depthai_ros_driver')

    ar = LaunchConfiguration('ar')
    ar_params = LaunchConfiguration('ar_params')
    front_name = LaunchConfiguration('front_name').perform(context)
    back_name = LaunchConfiguration('back_name').perform(context)
    gazebo = LaunchConfiguration('gazebo')
    oak_params = LaunchConfiguration('oak_params')
    bootie_params = LaunchConfiguration('bootie_params')
    pointclouds = LaunchConfiguration('pointclouds')

    return [
        # IncludeLaunchDescription(
        #     condition=UnlessCondition(gazebo),
        #     launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([depthai_dir, 'launch', 'camera.launch.py'])),
        #     launch_arguments={'name': front_name,
        #                       'params_file': oak_params,
        #                       'rectify_rgb': 'false',
        #                       }.items()
        # ),
        # IncludeLaunchDescription(
        #     condition=UnlessCondition(gazebo),
        #     launch_description_source=PythonLaunchDescriptionSource(PathJoinSubstitution([depthai_dir, 'launch', 'camera.launch.py'])),
        #     launch_arguments={'name': 'bootie',
        #                       'params_file': bootie_params,
        #                       'rectify_rgb': 'false',
        #                       }.items()
        # ),
        ComposableNodeContainer(
            name=f'{front_name}_image_proc_container',
            package='rclcpp_components',
            namespace='',
            executable='component_container',
            composable_node_descriptions=[
                ComposableNode(
                    condition=UnlessCondition(gazebo),
                    package="depthai_ros_driver",
                    plugin="depthai_ros_driver::Camera",
                    name=front_name,
                    parameters=[oak_params],
                ),
                ComposableNode(
                    condition=IfCondition(pointclouds),
                    package='rtabmap_util',
                    plugin='rtabmap_util::PointCloudXYZ',
                    name='point_cloud_xyz',
                    parameters=[{'decimation': 2,
                                'max_depth': 10.0,
                                'voxel_size': 0.1}],
                    remappings=[('depth/image', f'{front_name}/stereo/image_raw'),
                                ('depth/camera_info', f'{front_name}/stereo/camera_info'),
                                ('cloud', f'{front_name}/depth/points')],
                ),
            ],
        ),
        ComposableNodeContainer(
            name=f'{back_name}_image_proc_container',
            package='rclcpp_components',
            namespace='',
            executable='component_container',
            composable_node_descriptions=[
                ComposableNode(
                    condition=UnlessCondition(gazebo),
                    package="depthai_ros_driver",
                    plugin="depthai_ros_driver::Camera",
                    name=back_name,
                    parameters=[bootie_params],
                )
            ]
        ),
        Node(
            condition=IfCondition(ar),
            package='aruco_opencv',
            executable='aruco_tracker_autostart',
            arguments=['--ros-args', '--params-file', ar_params],
        ),
        # Node(
        #     package='imu_transformer',
        #     executable='imu_transformer_node',
        #     name='imu_transformer',
        #     remappings=[('/imu_in', '/oak/imu/data'),
        #                 ('/imu_out', '/oak/imu/transformed')],
        #     parameters=[{'target_frame': 'oak'}]
        # ),
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
            name='front_name',
            default_value='oak',
            description='',
        ),
        DeclareLaunchArgument(
            name='back_name',
            default_value='bootie',
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
