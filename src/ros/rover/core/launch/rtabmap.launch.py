import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import LoadComposableNodes, Node
from launch_ros.descriptions import ComposableNode

def launch_setup(context, *args, **kwargs):
    use_sim_time = LaunchConfiguration('use_sim_time')
    name = LaunchConfiguration('name').perform(context)
    qos = LaunchConfiguration("qos")
    core_prefix = get_package_share_directory('core')
    parameters={
          'frame_id':'oak',
          'use_sim_time':use_sim_time,
          'subscribe_rgb': True,
          'subscribe_depth':True,
          'subscribe_odom_info': True,
          'approx_sync':True,
          'odom_frame_id': 'odom',
          "Rtabmap/DetectionRate": "3.5",
    }

    remappings = [
        ("rgb/image", name+"/rgb/image_rect"),
        ("rgb/camera_info", name+"/rgb/camera_info"),
        ("depth/image", name+"/stereo/image_raw"),
        # ("imu", name+"/imu/data"),
        # ("odom", "odom/visual"),
    ]

    return [
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(core_prefix, 'launch', 'camera.launch.py')),
            condition = IfCondition(LaunchConfiguration('launch_camera')),
        ),

        LoadComposableNodes(
            target_container=name + "_container",
            composable_node_descriptions=[
                ComposableNode(
                    package='rtabmap_odom',
                    plugin='rtabmap_odom::RGBDOdometry',
                    name='rgbd_odometry',
                    parameters=[odom_parameters, {"publish_tf": True}],
                    remappings=remappings +
                               [('/odom', '/odom/visual')]
                    ,
                ),
          ],
        ),

        LoadComposableNodes(
            target_container=name + "_container",
            composable_node_descriptions=[
                ComposableNode(
                    package='rtabmap_slam',
                    plugin='rtabmap_slam::CoreWrapper',
                    name='rtabmap',
                    parameters=[parameters,
                                {'publish_tf':True}],
                    remappings=remappings,
                ),
            ],
        ),

        Node(
            package="rtabmap_viz",
            condition=IfCondition(LaunchConfiguration("rtabmap_viz")),
            executable="rtabmap_viz",
            output="screen",
            parameters=[parameters],
            remappings=remappings,
        ),

        Node(
        package='rtabmap_util',
        executable='point_cloud_xyz',
        condition=IfCondition(LaunchConfiguration('rtabmap_pointcloud')),
        name='rtabmap_point_cloud_xyz',
        remappings=[('/depth/image', name+'/stereo/image_raw'),
                    ('/depth/camera_info', name+'/stereo/camera_info'),
                    ('/cloud', name+'/rtabmap/points'),
                    ],
        ),

    ]
 

def generate_launch_description():
    depthai_prefix = get_package_share_directory("depthai_ros_driver")
    core_prefix = get_package_share_directory('core')
    declared_arguments = [
        DeclareLaunchArgument("name", default_value="oak"),
        #DeclareLaunchArgument("params_file", default_value=os.path.join(depthai_prefix, 'config', 'rgbd.yaml')),
        DeclareLaunchArgument("params_file", default_value=os.path.join(core_prefix, 'params', 'depthai_oakd_rgbd.yaml')),
        DeclareLaunchArgument("rectify_rgb", default_value="True"),
        DeclareLaunchArgument("rtabmap_viz", default_value="False"),
        DeclareLaunchArgument('use_sim_time', default_value='False'),
        DeclareLaunchArgument('rtabmap_pointcloud', default_value='True'),
        DeclareLaunchArgument('launch_camera', default_value='False'),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
