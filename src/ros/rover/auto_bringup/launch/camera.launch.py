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
    name = LaunchConfiguration('name').perform(context)
    depthai_prefix = get_package_share_directory("depthai_ros_driver")
    bringup_dir = get_package_share_directory('auto_bringup')

    params_file= LaunchConfiguration("params_file")
    
    use_sim_time = LaunchConfiguration('use_sim_time')

    return [
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                FindPackageShare([depthai_prefix, 'launch', 'camera.launch.py'])),
            launch_arguments={"name": name,
                              "parent_frame": "camera_link",
                              "params_file": params_file}.items()),

        LoadComposableNodes(
            condition=IfCondition(LaunchConfiguration("rectify_rgb")),
            target_container=name+"_container",
            composable_node_descriptions=[
                ComposableNode(
                    package='depth_image_proc',
                    plugin='depth_image_proc::PointCloudXyziNode',
                    name='point_cloud_xyzi',
                    remappings=[('depth/image_rect', name+'/stereo/image_raw'),
                                ('intensity/image_rect', name+'/right/image_rect'),
                                ('intensity/camera_info', name+'/stereo/camera_info'),
                                ('points', name+'/points')
                                ]),
            ]),

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
            arguments=['--ros-args', '--params-file', os.path.join(bringup_dir, 'params', 'aruco_tracker.yaml')],
        ),
        Node(
            condition=IfCondition(LaunchConfiguration('ar_tag')),
            package='nova_ar_tag',
            executable='aruco_marker',
            name='aruco_marker',
        ),
    ]


def generate_launch_description():
    auto_bringup_prefix = FindPackageShare('auto_bringup')
    declared_arguments = [
        DeclareLaunchArgument("name", default_value="oak"),
        DeclareLaunchArgument("params_file", default_value=PathJoinSubstitution([auto_bringup_prefix, 'params', 'depthai_oakd_rgbd.yaml'])),
        DeclareLaunchArgument("rectify_rgb", default_value="True"),
        DeclareLaunchArgument('use_sim_time', default_value='False'),
        DeclareLaunchArgument('rtabmap_pointcloud', default_value='True'),
        DeclareLaunchArgument('ar_tag', default_value='False'),
    ]

    return LaunchDescription(  
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
