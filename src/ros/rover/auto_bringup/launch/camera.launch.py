import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction, GroupAction
from launch.conditions import IfCondition, LaunchConfigurationNotEquals
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import LoadComposableNodes, Node, ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
from launch_ros.substitutions import FindPackageShare

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = get_package_share_directory('auto_bringup')
    depthai_prefix = get_package_share_directory('depthai_ros_driver')
    urdf_launch_dir = os.path.join(get_package_share_directory('depthai_descriptions'), 'launch')

    camera_model = LaunchConfiguration('camera_model')
    name = LaunchConfiguration('name').perform(context)
    namespace = LaunchConfiguration('namespace').perform(context)
    params_file = LaunchConfiguration('params_file')
    parent_frame = LaunchConfiguration('parent_frame').perform(context)
    rectify_rgb = LaunchConfiguration('rectify_rgb')
    use_camera = LaunchConfiguration('use_camera')
    yolo_file = LaunchConfiguration('yolo_file').perform(context)

    parameter_overrides = {
        "nn": {
            "i_nn_config_path": yolo_file,
        },
    }

    return [
        GroupAction(
            condition=IfCondition(use_camera),
            actions=[
                IncludeLaunchDescription(
                    PythonLaunchDescriptionSource(os.path.join(urdf_launch_dir, 'urdf_launch.py')),
                    launch_arguments={
                        'namespace': namespace,
                        'tf_prefix': name,
                        'camera_model': camera_model,
                        'base_frame': name,
                        'parent_frame': parent_frame,
                        'cam_pos_x': '0.0',
                        'cam_pos_y': '0.0',
                        'cam_pos_z': '0.0',
                        'cam_roll': '0.0',
                        'cam_pitch': '0.0',
                        'cam_yaw': '0.0',
                        'use_composition': 'true',
                        'use_base_descr': 'false',
                        'rs_compat': 'false',
                    }.items()
                ),
                ComposableNodeContainer(
                    name=f'{name}_container',
                    namespace=namespace,
                    package='rclcpp_components',
                    executable='component_container',
                    composable_node_descriptions=[
                        ComposableNode(
                            package='depthai_ros_driver',
                            plugin='depthai_ros_driver::Camera',
                            name=name,
                            namespace=namespace,
                            parameters=[
                                params_file,
                                parameter_overrides])],
                    arguments=['--ros-args', '--log-level', 'info'],
                    prefix=[''],
                    output='both'
                ),
                LoadComposableNodes(
                    condition=IfCondition(rectify_rgb),
                    target_container=f'{namespace}/{name}_container',
                    composable_node_descriptions=[
                        ComposableNode(
                            package='image_proc',
                            plugin='image_proc::RectifyNode',
                            name='rectify_color_node',
                            namespace=namespace,
                            remappings=[
                                ('image', f'{name}/rgb/image_raw'),
                                ('camera_info', f'{name}/rgb/camera_info'),
                                ('image_rect', f'{name}/rgb/image_rect'),
                                ('image_rect/compressed', f'{name}/rgb/image_rect/compressed'),
                                ('image_rect/compressedDepth', f'{name}/rgb/image_rect/compressedDepth'),
                                ('image_rect/theora', f'{name}/rgb/image_rect/theora')])],
                )]
        ),
        LoadComposableNodes(
            condition=IfCondition(rectify_rgb),
            target_container=f'{name}_container',
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
    auto_bringup_prefix = get_package_share_directory('auto_bringup')
    declared_arguments = [
        DeclareLaunchArgument('ar_tag', default_value='true'),
        DeclareLaunchArgument('camera_model', default_value='OAK-D-LR'),
        DeclareLaunchArgument('name', default_value='oak'),
        DeclareLaunchArgument('namespace', default_value=''),
        DeclareLaunchArgument('params_file', default_value=PathJoinSubstitution([auto_bringup_prefix, 'params', 'depthai_oakd_rgbd.yaml'])),
        DeclareLaunchArgument('parameter_overrides', default_value=''),
        DeclareLaunchArgument('parent_frame', default_value='camera_link'),
        DeclareLaunchArgument('rectify_rgb', default_value='true'),
        DeclareLaunchArgument('rtabmap_pointcloud', default_value='true'),
        DeclareLaunchArgument('use_camera', default_value='true'),
        DeclareLaunchArgument('yolo_file', default_value=os.path.join(auto_bringup_prefix, 'resources', 'URC_V5', 'URC_V5.json')),
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )