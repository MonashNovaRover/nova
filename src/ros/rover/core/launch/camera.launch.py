import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.conditions import IfCondition, LaunchConfigurationNotEquals
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import LoadComposableNodes, Node
from launch_ros.descriptions import ComposableNode

def launch_setup(context, *args, **kwargs):
    name = LaunchConfiguration('name').perform(context)
    depthai_prefix = get_package_share_directory("depthai_ros_driver")
    core_dir = get_package_share_directory('core')

    params_file= LaunchConfiguration("params_file")
    
    use_sim_time = LaunchConfiguration('use_sim_time')

    return [
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(depthai_prefix, 'launch', 'camera.launch.py')),
            launch_arguments={"name": name,
                              "parent_frame": "camera_link",
                              "params_file": params_file}.items()),

        LoadComposableNodes(
            condition=IfCondition(LaunchConfiguration("rectify_rgb")),
            target_container=name+"_container",
            composable_node_descriptions=[
                ComposableNode(
                    package="image_proc",
                    plugin="image_proc::RectifyNode",
                    name="rectify_color_node",
                    remappings=[('image', name+'/rgb/image_raw'),
                                ('camera_info', name+'/rgb/camera_info'),
                                ('image_rect', name+'/rgb/image_rect'),
                                ('image_rect/compressed', name+'/rgb/image_rect/compressed'),
                                ('image_rect/compressedDepth', name+'/rgb/image_rect/compressedDepth'),
                                ('image_rect/theora', name+'/rgb/image_rect/theora')]
                )
            ]),
        
        Node(
            package='rtabmap_util',
            executable='point_cloud_xyz',
            condition=IfCondition(LaunchConfiguration('rtabmap_pointcloud')),
            name='rtabmap_point_cloud_xyz',
            remappings=[('/depth/image', name+'/stereo/image_filtered'),
                        ('/depth/camera_info', name+'/stereo/camera_info'),
                        ('/cloud', name+'/rtabmap/points'),
                        ],
            parameters=[{'min_depth': 1.4, 'filter_nans':True }] 
        ),
        Node(
            package='nova_pointcloud_filter',
            executable='depth_filter',
            name='depth_filter',
            remappings=[('/depth/image', name+'/stereo/image_raw'),
                        ('/depth/image_filtered', name+'/stereo/image_filtered'),
                        ],
            parameters=[{'t_filter': 0, 'r_filter': 0, 'b_filter': 0, 'l_filter': 0, }] 
        ),
        Node(
            condition=IfCondition(LaunchConfiguration('ar_tag')),
            package='aruco_opencv',
            executable='aruco_tracker_autostart',
            arguments=['--ros-args', '--params-file', os.path.join(core_dir, 'params', 'aruco_tracker.yaml')],
        ),
        Node(
            condition=IfCondition(LaunchConfiguration('ar_tag')),
            package='nova_ar_tag',
            executable='aruco_marker',
            name='aruco_marker',
        ),
    ]


def generate_launch_description():
    depthai_prefix = get_package_share_directory("depthai_ros_driver")
    core_prefix = get_package_share_directory('core')
    declared_arguments = [
        DeclareLaunchArgument("name", default_value="oak"),
        DeclareLaunchArgument("params_file", default_value=os.path.join(core_prefix, 'params', 'depthai_oakd_rgbd.yaml')),
        DeclareLaunchArgument("rectify_rgb", default_value="True"),
        DeclareLaunchArgument('use_sim_time', default_value='False'),
        DeclareLaunchArgument('rtabmap_pointcloud', default_value='True'),
        DeclareLaunchArgument('ar_tag', default_value='False'),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
