from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration,PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.descriptions import ComposableNode
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import ComposableNodeContainer

def launch_setup(context, *args, **kwargs):
    auto_bringup_dir = FindPackageShare('auto_bringup')

    use_sim_time = LaunchConfiguration('use_sim_time')
    name = LaunchConfiguration('name').perform(context)
    x = LaunchConfiguration('x').perform(context)
    y = LaunchConfiguration('y').perform(context)
    z = LaunchConfiguration('z').perform(context)
    roll = LaunchConfiguration('roll').perform(context)
    pitch = LaunchConfiguration('pitch').perform(context)
    yaw = LaunchConfiguration('yaw').perform(context)
    camera = LaunchConfiguration('camera').perform(context)
    rtabmap_viz = LaunchConfiguration('rtabmap_viz').perform(context)

    parameters={
          'frame_id':'base_link',
          'use_sim_time':use_sim_time,
          'subscribe_rgb': True,
          'subscribe_depth':True,
          'subscribe_odom_info': False,
          'approx_sync':True,
          'odom_frame_id': 'odom',
          'Rtabmap/DetectionRate': '3.5',
    }

    remappings = [
        ('rgb/image', name+'/rgb/image_raw'),
        ('rgb/camera_info', name+'/rgb/camera_info'),
        ('depth/image', name+'/stereo/image_raw'),
        # ('imu', name+'/imu/data'),
        ('odom', 'odom/visual'),
    ]

    return [
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(PathJoinSubstitution([auto_bringup_dir, 'launch', 'camera.launch.py'])),
            condition=IfCondition(camera),
            launch_arguments={
                'pointclouds':'False',
                'gazebo': use_sim_time
            }.items()
        ),
        ComposableNodeContainer(
            name=f"{name}_image_mapping_container",
            namespace='',
            package="rclcpp_components",
            executable="component_container",
            composable_node_descriptions=[
                ComposableNode(
                    package='rtabmap_odom',
                    plugin='rtabmap_odom::RGBDOdometry',
                    name='rgbd_odometry',
                    parameters=[parameters, {'publish_tf': False, 'initial_pose': f'{x} {y} {z} {roll} {pitch} {yaw}','publish_null_when_lost': False}],
                    remappings=remappings,
                ),
                ComposableNode(
                    package='rtabmap_slam',
                    plugin='rtabmap_slam::CoreWrapper',
                    name='rtabmap',
                    parameters=[parameters,
                                {'publish_tf':True, 'rtabmap_args':'--delete_db_on_start'}],
                    remappings=remappings,
                )
            ]
        ),
        Node(
            package='rtabmap_viz',
            condition=IfCondition(rtabmap_viz),
            executable='rtabmap_viz',
            output='screen',
            parameters=[parameters],
            remappings=remappings,
        ),
    ]


def generate_launch_description():
    declared_arguments = [
        DeclareLaunchArgument(name='name', default_value='oak'),
        DeclareLaunchArgument(name='rtabmap_viz', default_value='False'),
        DeclareLaunchArgument(name='use_sim_time', default_value='False'),
        DeclareLaunchArgument(name='camera', default_value='False'),
        DeclareLaunchArgument(name='x', default_value='0.0'),
        DeclareLaunchArgument(name='y', default_value='0.0'),
        DeclareLaunchArgument(name='z', default_value='0.0'),
        DeclareLaunchArgument(name='roll', default_value='0.0'),
        DeclareLaunchArgument(name='pitch', default_value='0.0'),
        DeclareLaunchArgument(name='yaw', default_value='0.0'),
    ]

    return LaunchDescription(
        declared_arguments + [OpaqueFunction(function=launch_setup)]
    )
